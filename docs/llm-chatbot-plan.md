# Entwicklungsplan: LLM-Chatbot auf Basis von LM Studio

## Ziel

Zusätzlich zum bestehenden, regelbasierten Chatbot (`src/chatbot/`, Steps/Threads mit
klassischen Eingabeelementen) soll ein zweiter Chat-Modus angeboten werden, der ein
lokal laufendes LLM über **LM Studio** nutzt. Die GUI prüft beim Start, ob eine
LM-Studio-Installation vorhanden ist. Nur dann wird der LLM-Chat mit freier
Texteingabe angeboten. Das LLM steuert die GUI über eine neue
**GUI-Aktionsschicht** (Tools), die auf denselben Zustand wirkt, den die GUI
anzeigt — so wie es die klassischen Chatbot-Steps heute schon tun.

> **Wichtige Abgrenzung:** Die vorhandenen MCP-Tools des qapla-engine-testers
> (`src/mcp/`) sind dafür **nicht** wiederverwendbar. Sie führen Aufgaben über
> `AppRunner::runDispatcher()` aus — den headless CLI-Runner mit eigenen
> `Tournament`-Objekten. Ein so gestartetes Turnier liefe unsichtbar neben der
> GUI her; `TournamentData::instance()` (der Zustand, den die GUI rendert)
> bliebe unberührt. Vom MCP-Code wird deshalb nur das Schema-Format
> übernommen, nicht die Ausführung.

## Ausgangslage (Ist-Zustand)

| Baustein | Zustand |
|---|---|
| Klassischer Chatbot | `ChatbotWindow` (Singleton) mit registrierbaren `ChatbotThread`-Prototypen und `ChatbotStep`-Pattern (siehe `docs/code-doku/CHATBOT_STEP_PATTERN.md`) |
| MCP-Server | `extern/qapla-engine-tester/src/mcp/` — `McpServer` mit `listTools`, `callTool`, `listResources`. **Achtung:** Die Tool-Ausführung läuft über den headless `AppRunner` und wirkt nicht auf den GUI-Zustand — nur Schema-Format/`mcp-schema-builder` sind wiederverwendbar |
| GUI-Zustand | Singletons wie `TournamentData::instance()`, `SprtTournamentData::instance()`, `InteractiveBoardWindow` — die Fenster rendern jedes Frame direkt daraus; die klassischen Chatbot-Steps steuern die GUI über genau diese Singletons |
| HTTP-Client | Nicht vorhanden — muss ergänzt werden |
| LM Studio | Bietet eine OpenAI-kompatible REST-API (Default `http://localhost:1234/v1`) mit `GET /v1/models` und `POST /v1/chat/completions` inkl. Tool-/Function-Calling; CLI `lms` unter `~/.lmstudio/bin/` |

## Architekturüberblick

```
+------------------------------------------------------------------+
| ChatbotWindow (bestehend)                                        |
|   +-- ChatbotThread "Tournament" (bestehend, klassisch)          |
|   +-- ChatbotThread "Board"      (bestehend, klassisch)          |
|   +-- LlmChatThread  "KI-Chat"   (NEU, nur wenn LM Studio da)    |
+------------------------------|-----------------------------------+
                               v
                    +---------------------+
                    | LlmChatController   |  Konversation, Agenten-Loop,
                    | (Worker-Thread)     |  Nachrichtenhistorie
                    +----|-----------|----+
                         v           v
              +-------------+   +--------------------+
              | LmStudio-   |   | GuiToolRegistry    |
              | Client      |   | (Job-Queue → UI-   |
              | (HTTP/JSON) |   |  Thread)           |
              +------|------+   +---------|----------+
                     v                    v
             LM Studio Server    +---------------------------+
             localhost:1234      | GUI-Zustand (Singletons): |
             (OpenAI-API)        | TournamentData,           |
                                 | SprtTournamentData,       |
                                 | InteractiveBoardWindow, … |
                                 +---------------------------+
                                          ^
                                          | rendert jedes Frame
                                 +---------------------------+
                                 | Turnier-/Board-Fenster    |
                                 +---------------------------+
```

Kernidee: Das LLM erhält eine kuratierte Menge von **GUI-Tools** als
OpenAI-"tools" (Function-Calling). Antwortet das Modell mit `tool_calls`, führt
die `GuiToolRegistry` diese auf dem UI-Thread aus — auf **denselben Singletons,
aus denen die GUI jedes Frame rendert** (`TournamentData::instance()` usw.,
exakt die Codepfade der klassischen Chatbot-Steps, z. B.
`TournamentData::instance().startTournament()`). Dadurch ist garantiert, dass
jede LLM-Aktion sofort und korrekt in der GUI sichtbar ist: laufende Partien,
Elo-Tabelle, Statusanzeigen. Die Ergebnisse gehen als Tool-Messages zurück an
das LLM, bis eine finale Textantwort entsteht.

## Neue Module

Vorgeschlagene Ablage: `src/llm/` (Backend) und `src/chatbot/` (UI-Anbindung).

### 1. `LmStudioLocator` — Erkennung der Installation

Verantwortung: Herausfinden, ob LM Studio installiert ist und ob der Server läuft.

Erkennungsreihenfolge:
1. **Server-Probe**: `GET http://localhost:1234/v1/models` mit kurzem Timeout
   (~250 ms). Erfolg ⇒ Server läuft, Modellliste liegt gleich mit vor.
2. **Installations-Probe** (wenn Server nicht läuft), pro Plattform:
   - macOS: `/Applications/LM Studio.app`, `~/.lmstudio/` (enthält `bin/lms`)
   - Windows: `%LOCALAPPDATA%\Programs\LM Studio`, `%USERPROFILE%\.lmstudio`
   - Linux: `~/.lmstudio/`, AppImage-Fundorte nicht zuverlässig ⇒ nur `~/.lmstudio`
3. Optional: Wenn installiert, aber Server aus, kann die GUI anbieten, den Server
   per `lms server start` zu starten (Konfigurationsoption, nicht automatisch).

Ergebnis-Enum: `NotInstalled | InstalledServerDown | ServerRunning`.
Die Probe läuft asynchron beim GUI-Start (kein Blockieren des Renderings);
Host/Port sind über die Konfiguration überschreibbar (Default `localhost:1234`).

### 2. `LmStudioClient` — HTTP-Anbindung

- Neue Abhängigkeit: **cpp-httplib** (header-only, MIT) unter `extern/` — kleinster
  Eingriff ins Buildsystem; TLS wird nicht benötigt (nur localhost).
- API-Methoden:
  - `listModels()` → `GET /v1/models`
  - `chatCompletion(request)` → `POST /v1/chat/completions` mit `messages`,
    `tools`, `tool_choice`, optional `stream: true` (SSE) für tokenweise Ausgabe.
- JSON-Verarbeitung über den bereits vorhandenen `JsonValue`/`json-helper` des
  Engine-Testers — keine zusätzliche JSON-Bibliothek.
- Streaming ist ab Schritt 7 vorgesehen; zunächst reicht die blockierende Variante
  im Worker-Thread.

### 3. `GuiToolRegistry` — GUI-Aktionsschicht als LLM-Tools

Verworfene Alternativen (beide steuern **nicht** die GUI):
- *MCP-Tools in-process aufrufen*: `McpServer::callTool` führt über
  `AppRunner::runDispatcher()` den headless CLI-Runner aus — eigener
  `Tournament`-Zustand, die GUI-Fenster (die aus `TournamentData::instance()`
  rendern) zeigen davon nichts.
- *qapla-engine-tester als Subprozess mit MCP über stdio*: gleiches Problem,
  zusätzlich Prozessgrenze.

**Gewählter Ansatz:** Eine Registry von Tools, deren Handler exakt die
Operationen ausführen, die auch die klassischen Chatbot-Steps und die Fenster
selbst benutzen — d. h. auf den GUI-Singletons arbeiten. Damit ist die korrekte
Anzeige konstruktionsbedingt gewährleistet: Es gibt nur einen Zustand, und die
GUI rendert ihn jedes Frame neu (immediate mode).

Tool-Struktur (jeweils `name`, `description`, JSON-Schema, Handler):

| Tool (Beispiele) | Wirkt auf |
|---|---|
| `list_engines`, `select_engines` | Engine-Konfiguration / `ImGuiEngineSelect` der Ziel-Instanz |
| `configure_tournament` (Zeitkontrolle, Runden, Openings, PGN) | `TournamentData::instance().config()` + UI-Komponenten |
| `start_tournament`, `stop_tournament` | `TournamentData::instance().startTournament()` / `stopPool()` |
| `get_tournament_status` | `isRunning()`, `getPlayedGames()`, `getTournamentResult()` |
| `configure_sprt`, `start_sprt`, … | `SprtTournamentData::instance()` |
| `board_set_position`, `board_start_analysis`, … | `InteractiveBoardWindow` (per Board-ID, analog zum Provider-Pattern aus `CHATBOT_STEP_PATTERN.md`) |

Weitere Punkte:
- **Wiederverwendung aus dem MCP-Code**: Nur das Format der Tool-Schemata
  (JSON Schema) und ggf. der `mcp-schema-builder` als Hilfsmittel zum Erzeugen
  der Schemata — nicht die Tool-Handler.
- **Schema-Konvertierung**: Tool-Definitionen → OpenAI-Function-Calling-Format
  (`{"type": "function", "function": {name, description, parameters}}`).
- **Thread-Sicherheit**: Tool-Aufrufe kommen aus dem LLM-Worker-Thread, die
  Handler müssen aber auf dem UI-/Hauptthread laufen (sie fassen GUI-Zustand
  an). Deshalb Job-Queue: Der Worker stellt den Aufruf ein, der Hauptthread
  arbeitet ihn im Frame-Loop ab, der Worker wartet mit Timeout auf das Ergebnis.
- **Zielobjekt-Auflösung**: Wie beim Step-Pattern liefern Provider-Callbacks
  Zeiger auf das Ziel (Board kann geschlossen sein ⇒ nullptr ⇒ Fehler-Result
  ans LLM statt Crash).
- **Absicherung**: Whitelist der freigegebenen Tools, Iterationslimit pro
  Nutzeranfrage (z. B. max. 10 Tool-Runden), Timeouts pro Tool-Aufruf.

### 4. `LlmChatController` — Konversation und Agenten-Loop

- Hält die Nachrichtenhistorie (`system`, `user`, `assistant`, `tool`).
- **System-Prompt**: beschreibt die Schach-GUI, die verfügbaren Tools, Antwortsprache
  (aus der i18n-Einstellung der GUI) und Verhaltensregeln ("frage nach, bevor du
  ein laufendes Turnier stoppst" o. Ä.).
- Agenten-Loop pro Nutzereingabe:
  1. `chat/completions` mit Historie + Tools aufrufen.
  2. Enthält die Antwort `tool_calls`: Tools über die `GuiToolRegistry` ausführen,
     Ergebnisse als `tool`-Messages anhängen, zurück zu 1.
  3. Sonst: finale Antwort in die Historie und an die UI.
- Läuft vollständig in einem **Worker-Thread**; Kommunikation mit der UI über
  eine thread-sichere Queue (Status: „denkt…", „führt Tool X aus…", Antwort­text,
  Fehler). Abbruch-Flag für einen „Stop"-Button.
- Kontextfenster-Verwaltung: Historie bei Überlänge kürzen (älteste Turns
  entfernen, System-Prompt behalten).

### 5. `LlmChatThread` — UI-Integration in den bestehenden Chatbot

- Neue `ChatbotThread`-Ableitung, registriert über
  `ChatbotWindow::registerThread()` — **nur wenn** der `LmStudioLocator` mindestens
  `InstalledServerDown` meldet.
- Ein einzelner, dauerhafter Step mit:
  - Scrollbarer Nachrichtenhistorie (User-/Assistant-Blasen, Tool-Aufrufe
    kompakt als „⚙ Turnier gestartet" o. Ä.)
  - `ImGui::InputTextMultiline` + Senden-Button (Enter zum Senden)
  - Modell-Auswahl (Dropdown aus `listModels()`), Statuszeile, Stop-Button
  - Zustand `InstalledServerDown`: Hinweistext + Button „LM Studio Server starten"
- i18n: alle neuen Texte über das vorhandene Übersetzungsmodul (`i18n/`).

## Konfiguration

Neue Sektion in der bestehenden Konfiguration (z. B. `[llmchat]`):

- `enabled` (Default: auto = anbieten, wenn gefunden)
- `host`, `port` (Default `localhost:1234`)
- `model` (zuletzt gewähltes Modell)
- `maxToolIterations`, `requestTimeoutSeconds`

## Entwicklungsschritte

Jeder Schritt endet mit einem **sichtbar nutzbaren Ergebnis** in der GUI
(„Fertig heißt: …") und ist einzeln abnehmbar. Die Reihenfolge ist so gewählt,
dass jeder Schritt auf dem vorherigen aufbaut und nichts auf Vorrat gebaut wird.

### Schritt 1 — LM Studio wird erkannt, Chat-Fenster wird angeboten

1. cpp-httplib nach `extern/` holen, CMake-Einbindung.
2. `LmStudioLocator`: asynchrone Server-Probe (`GET /v1/models`) +
   Plattformpfad-Prüfung; Ergebnis-Enum `NotInstalled | InstalledServerDown |
   ServerRunning`.
3. `LlmChatThread`-Rumpf: wird im `ChatbotWindow` **nur registriert, wenn**
   LM Studio gefunden wurde. Zeigt zunächst nur eine Statuszeile
   („LM Studio erkannt, Server läuft" / „…installiert, Server nicht gestartet")
   und eine noch inaktive Texteingabe.
4. Konfigurationssektion `[llmchat]` (`host`, `port`, `enabled`).
5. Tests: Locator mit gemockten Pfaden/Mock-Server (Catch2); Registrierung
   an/aus je nach Locator-Ergebnis (imgui_test_engine).

**Fertig heißt:** Auf einem Rechner mit LM Studio erscheint der Eintrag
„KI-Chat" im Chatbot-Menü mit korrektem Status; ohne LM Studio erscheint er
nicht.

### Schritt 2 — LM Studio ist angebunden, Frage/Antwort-Chat funktioniert

1. `LmStudioClient`: `listModels()`, blockierendes `chatCompletion()` (noch ohne
   Tools); Test gegen Mock-HTTP-Server (cpp-httplib kann auch Server spielen).
2. `LlmChatController` (erste Ausbaustufe): Worker-Thread, Nachrichtenhistorie,
   System-Prompt, thread-sichere UI-Queue, Abbruch-Flag — noch ohne Tool-Loop.
3. Chat-UI im `LlmChatThread`: scrollbare Historie, Eingabefeld + Senden,
   Modell-Dropdown (aus `listModels()`), Statuszeile („denkt…"), Stop-Button.
4. Fehlerbilder: Server weg, kein Modell geladen, Timeout — verständliche
   Meldung im Chat.

**Fertig heißt:** Man kann im Chat-Fenster frei mit dem lokalen Modell
plaudern — Frage eintippen, Antwort erscheint; Modellwechsel per Dropdown.

### Schritt 3 — Erste GUI-Aktion: Engines mit Hilfe des LLM einrichten

1. `GuiToolRegistry`-Grundgerüst: Tool-Definition (Name, Beschreibung,
   JSON-Schema, Handler), Job-Queue zur Ausführung im UI-Thread,
   Schema-Export im OpenAI-Format, Argument-Validierung (Fehler als
   Tool-Result, nicht als Exception).
2. Agenten-Loop im `LlmChatController` aktivieren: `tool_calls` ausführen,
   Ergebnisse zurück ans LLM, Iterationslimit.
3. Erste, kleine Tool-Gruppe **Engine-Verwaltung**:
   - `list_installed_engines` — liest die vorhandene Engine-Konfiguration
   - `open_add_engine_dialog` — öffnet den Datei-Dialog der GUI zum
     Hinzufügen einer Engine (der Nutzer wählt die Datei selbst — bewusst
     kein Dateisystemzugriff durch das LLM)
4. Anzeige der Tool-Aufrufe im Chat („⚙ Datei-Dialog geöffnet").

**Fertig heißt:** „Ich möchte eine neue Engine einrichten" im Chat ⇒ der
Datei-Dialog der GUI öffnet sich; nach der Auswahl taucht die Engine in der
Engine-Liste der GUI auf und das LLM bestätigt es (via
`list_installed_engines`).

### Schritt 4 — Turnier per Chat starten, sichtbar in der GUI

1. Tool-Gruppe **Turnier**: `select_engines`, `configure_tournament`
   (Zeitkontrolle, Runden, Openings, PGN), `start_tournament` — Handler rufen
   dieselben `TournamentData`-Methoden wie die klassischen Chatbot-Steps.
2. Test (ohne LLM, direkt über die Registry): `start_tournament` ⇒
   `TournamentData::instance().isRunning() == true` **und** das Turnierfenster
   zeigt das laufende Turnier.
3. System-Prompt um Turnier-Wissen erweitern (welche Engines, sinnvolle
   Defaults, nachfragen bei fehlenden Angaben).

**Fertig heißt:** „Starte ein Turnier zwischen Engine A und B mit 1+0.5" im
Chat ⇒ das Turnierfenster zeigt das laufende Turnier mit Partien, Elo-Tabelle
und Status — exakt wie bei manueller Bedienung.

### Schritt 5 — Turnier überwachen und stoppen per Chat

1. Tools `get_tournament_status` (`isRunning()`, `getPlayedGames()`,
   `getTournamentResult()`) und `stop_tournament` (`stopPool()`, graceful).
2. Verhaltensregel im System-Prompt: vor dem Stoppen eines laufenden Turniers
   nachfragen.

**Fertig heißt:** „Wie steht das Turnier?" liefert Stand und Zwischenergebnis;
„Stoppe das Turnier" hält es an — sichtbar am Status im Turnierfenster.

### Schritt 6 — Weitere Bereiche: SPRT und Bretter

1. Tool-Gruppe **SPRT** analog zu Schritt 4/5 (`SprtTournamentData::instance()`).
2. Tool-Gruppe **Board**: `board_set_position`, `board_start_analysis`, … —
   Zielauflösung per Board-ID über Provider-Callbacks (Pattern aus
   `CHATBOT_STEP_PATTERN.md`; Board geschlossen ⇒ Fehler-Result statt Crash).

**Fertig heißt:** Ein SPRT-Test bzw. eine Brett-Analyse lässt sich vollständig
per Chat starten und ist im jeweiligen Fenster sichtbar. (Bei Bedarf in 6a/6b
teilbar — jede Tool-Gruppe ist für sich abnehmbar.)

### Schritt 7 — Komfort und Feinschliff

1. SSE-Streaming für tokenweise Anzeige der Antworten.
2. „LM Studio Server starten"-Button (`lms server start` als Subprozess) für
   den Zustand `InstalledServerDown`.
3. Kontextkürzung bei langer Historie, Konversation zurücksetzen.
4. i18n aller Texte vervollständigen; imgui_test_engine-Tests für die Chat-UI
   (Mock-Controller statt echtem LLM).
5. Dokumentation in `docs/code-doku/` (Architektur + wie man neue Tools ergänzt).

**Fertig heißt:** Antworten erscheinen tokenweise, der Server lässt sich aus
der GUI starten, und neue Tool-Gruppen sind dokumentiert erweiterbar.

## Risiken und offene Punkte

- **Tool-Calling-Qualität** hängt stark vom lokal geladenen Modell ab (kleine
  Modelle halluzinieren Parameter). Gegenmaßnahmen: strikte Schema-Validierung
  vor der Ausführung, verständliche Fehlermeldungen als Tool-Result, Hinweis in
  der UI auf empfohlene Modelle (z. B. Qwen- oder Llama-Instruct-Varianten mit
  Tool-Support).
- **Nebenläufigkeit**: Tool-Aufrufe verändern GUI-Zustand, während der Nutzer
  parallel klicken kann. Die Job-Queue im UI-Thread serialisiert das; destruktive
  Tools (Turnier stoppen, Engine entfernen) ggf. mit Bestätigungsdialog.
- **Portfreiheit**: LM Studio kann auf einem anderen Port laufen — deshalb
  konfigurierbar; die Probe prüft nur den konfigurierten Port.
- **Lizenz/Build**: cpp-httplib ist MIT und header-only — unkritisch; keine
  Laufzeitabhängigkeit zu LM Studio (reines HTTP).

## Aufwandsschätzung (grob)

| Schritt | Aufwand |
|---|---|
| 1 — Erkennung + Chat-Fenster | 2 Tage |
| 2 — Frage/Antwort-Chat | 2–3 Tage |
| 3 — Registry + Engine-Einrichtung | 3–4 Tage (enthält das Tool-Grundgerüst) |
| 4 — Turnier starten | 2–3 Tage |
| 5 — Überwachen/Stoppen | 1 Tag |
| 6 — SPRT + Bretter | 2–3 Tage |
| 7 — Feinschliff | 3–4 Tage |
