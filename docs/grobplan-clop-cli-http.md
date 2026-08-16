# Grobplan: CLOP im Chat, CLI-Fähigkeit, HTTP-Schnittstelle

## Ziel

Von der KI gestartete Läufe sollen in der GUI **sichtbar** sein: laufende Partien auf den
Brettern, Ergebnistabellen, Status. Heute läuft die Zusammenarbeit über den
qapla-engine-tester (CLI, Dateien) — das ist für die KI bequem, aber zum Zuschauen
schlecht. Die drei Punkte unten sind drei Wege, das zu ändern.

Der Plan bewertet **nicht den Aufwand**, sondern zwei andere Fragen:

- **Umständlichkeit** — wie viel Reibung entsteht beim Bauen und beim Benutzen?
- **Neuheitsgrad** — ist das wirklich etwas Neues, oder ein vorhandenes Muster noch einmal
  angewendet (so wie SPRT sich zum Turnier verhält)?

## Ausgangslage, die für alle drei Punkte gilt

| Fakt | Wo |
|---|---|
| **Alle** Quellen des Engine-Testers werden ins GUI-Binary kompiliert (nur `qapla-engine-tester.cpp` ist ausgeschlossen) | `CMakeLists.txt:97-105` |
| Damit sind `CLOPOptimizer`, `SPSAOptimizer`, `AppRunner`, `Settings::Manager`, `McpServer` bereits vorhanden — nichts davon ist eine neue Abhängigkeit | `extern/qapla-engine-tester/src/{clop,spsa,cli,mcp}/` |
| cpp-httplib ist Submodul und CMake-Ziel, bisher nur als Client genutzt (LM Studio); die Unit-Tests nutzen ihn bereits als **Server** | `extern/httplib`, `src/llm/lm-studio-client.cpp`, `src/test-system/unit/lm-studio-client-test.cpp` |
| Die GUI hat eine saubere Dreiteilung: Tools (JSON, modellseitig) → Aktionen (reines C++) → GUI-Singletons | `src/llm/tools/`, `src/llm/actions/`, `src/llm/gui-tool-registry.h` |
| Bretter werden **generisch aus dem Pool** befüllt (`withGameRecords` über `gameIndex`), nicht pro Betriebsart | `src/viewer-board-window-list.h:66` |
| Der klassische Chat kennt keine Freitexteingabe: Steps sind Optionslisten plus eingebettete GUI-Steuerelemente | `src/chatbot/chatbot-step-option-list.h`, `chatbot-step-sprt-configuration.cpp` |

Die Abgrenzung aus `docs/llm-chatbot-plan.md` gilt weiterhin und ist für Punkt 2 und 3 der
entscheidende Punkt: **`AppRunner` führt Aufgaben mit eigenen `Tournament`-/`SprtManager`-Objekten
aus.** Der Zustand, den die GUI jedes Frame rendert (`TournamentData::instance()` usw.), bleibt
dabei unberührt. Verschärfend kommt hinzu, dass `AppRunner` denselben
`GameManagerPool::getInstance()` benutzt wie die GUI (`GameManagerPoolAccess` fällt ohne
expliziten Pool auf das Singleton zurück) — ein per `AppRunner` gestarteter Lauf würde also
im selben Pool Plätze belegen, ohne dass die GUI-Tabellen etwas davon wissen.

---

## 1. CLOP in die GUI, nur über den Chat

### Idee

Kein eigener Tab. Ein neuer `ChatbotThread` im klassischen Chat, plus Tools für den KI-Chat.

### Was schon da ist

- **Der Optimierer selbst**: `CLOPOptimizer::createCLOP(engines, config)` /
  `scheduleCLOP(concurrency, pool)` / `stop()` / `getEstimatedParameters()` /
  `getCompletedSamples()` / `getStatusTable()`. Das ist fast dieselbe Form wie
  `SprtManager::schedule(manager, concurrency, pool)`, wie ihn
  `SprtTournamentData::startTournament()` bedient.
- **Das Datenmuster**: `SprtTournamentData` (Singleton, hält Manager + Config +
  `ViewerBoardWindowList` + `GameManagerPoolAccess`, `pollData()` im Frame-Loop,
  `State`-Enum, Speichern/Laden über `SprtTournamentFile`). Ein `ClopData` wäre eine
  Kopie dieses Musters; `ClopFile::id = "clop"` existiert bereits als Format-Kennung.
- **Die Partieanzeige**: nichts zu tun. Sobald CLOP im selben Pool läuft, füllt
  `ViewerBoardWindowList::populateViews()` die Bretter — es liest Partien über den
  Pool-Index, nicht über die Betriebsart.
- **Der klassische Chat-Ablauf**: `src/chatbot/sprt/` ist die Vorlage — Thread + Steps
  (Engines wählen, konfigurieren, starten, Ergebnis).
- **Die KI-Tools**: `defineTool<Request>()` in `src/llm/tools/` plus Aktionen in
  `src/llm/actions/`. Die betriebsartübergreifenden Tools (`start`, `stop`, `get_status`,
  `clear_result`, `show_result`) nehmen bereits ein `type`-Feld — CLOP wäre ein vierter
  Enum-Wert in `Actions::Activity`, kein neuer Tool-Satz.
- **Parametergrenzen**: `EngineOption` trägt `min`/`max`, und `engine-parameter-bounds`
  existiert im Tester. Die Auswahl der zu tunenden Engine-Optionen kann daraus vorbelegt
  werden statt frei getippt zu werden.

### Was wirklich neu ist

1. **Parameterliste variabler Länge.** Alle bisherigen Konfigurationen (Turnier, SPRT, EPD)
   sind feste Felder. CLOP braucht *n* Einträge `{Name, min, max}`. Zwei Stellen sind
   betroffen:
   - GUI: ein neues Steuerelement analog `ImGuiSprtConfiguration` (Zeile hinzufügen/löschen,
     Vorbelegung aus den UCI-Optionen der gewählten Engine).
   - Tool-API: `Api::ParamType` kennt heute nur Skalare und `StringArray`. Entweder eine
     Kodierung als String-Array (`"Margin:0:200"`) — billig, aber für kleine Modelle
     fehleranfällig — oder ein neuer Parametertyp für Objektlisten.
2. **Ergebnisanzeige.** CLOP liefert `TableData` (`getStatusTable()`). Einen
   `TableData` → `ImGuiTable`-Adapter gibt es noch nicht; bisher wird `TableData` nur über
   `TableFormat::toText()` als Text ausgegeben (`tournament-result-view.cpp:329`). Der Adapter
   ist klein und danach für alles Weitere nutzbar.

### Bewertung

**Umständlichkeit: gering.** **Neuheitsgrad: niedrig** — im Kern die dritte Anwendung des
Musters „Daten-Singleton + Chat-Thread + Tool-Gruppe". Zwei echte Neuteile (Parameterliste,
Tabellen-Adapter), beide klein und beide danach wiederverwendbar.

**Nebeneffekt:** `SPSAOptimizer` hat dieselbe Bauform. Steht CLOP, ist SPSA gleichartig.

**Ein inhaltlicher Punkt, keine technische Hürde:** CLOP läuft lange und ändert laufend die
Schätzwerte. „Zuschauen" heißt hier vor allem, dass die Statustabelle regelmäßig aktualisiert
im Chat oder in einem Ergebnisbereich steht — das ist genau das, was `renderWidget` in
`GuiToolResult` schon kann (das Widget wird jedes Frame neu gezeichnet, solange der
Chat-Eintrag sichtbar ist).

---

## 2. CLI des Engine-Testers auch in der GUI nutzen

### Idee

Dieselbe Kommandozeile/Konfiguration wie beim Engine-Tester an die GUI übergeben, und die GUI
startet daraus die passende Aktion.

### Wo es trägt

Die **Parserseite** ist ohne Einschränkung nutzbar und schon im Binary:

- `Settings::Manager` (Singleton) mit `settings-definitions.cpp` — Kommandozeile und
  `.ini`-artige Dateien werden zu `GroupInstance`-Blöcken.
- Die Dateiformate sind bereits gemeinsam: die GUI speichert und lädt über
  `TournamentFile` bzw. `SprtTournamentFile` (`tournament-data.cpp:849`,
  `sprt-tournament-data.cpp:720`), und `ConfigGroupLoader` füllt daraus GUI-Objekte.
  Ein `.qtour` aus der GUI und eines vom Tester sind dasselbe.
- `Cli::TaskType` (`sprt`, `tournament`, `epd`, `spsa`, `clop`, …) benennt die Betriebsart
  bereits so, wie die GUI sie bräuchte.

### Wo es nicht trägt

**Die Ausführungsseite ist genau die, die man nicht nehmen darf.** `AppRunner::runDispatcher()`
ist der Kern der CLI — und er baut eigene Manager-Objekte auf. Ein so gestarteter Lauf wäre in
der GUI *halb* sichtbar, was schlechter ist als gar nicht:

- Bretter: würden Partien zeigen (Pool-basiert),
- Turnierfenster, Elo-Tabelle, Status, Stopp-Knöpfe: blieben leer bzw. wirkungslos,
- Nebenläufigkeit: zwei Instanzen (CLI-Lauf und ein GUI-Lauf) würden sich denselben Pool
  teilen, ohne voneinander zu wissen.

Zusätzlich belegt `InputHandler::inputLoop()` `stdin` mit einem eigenen Thread — in einer GUI,
die von außen gestartet wird, ist das kein brauchbarer Kanal.

### Was zu tun wäre

Ein **Mapper** `GroupInstance` → GUI-Zustand und ein **eigener Dispatcher**, der statt
`AppRunner::run*` die vorhandene Aktionsschicht aufruft:

```
CLI-Text (argv / Datei)
  → Settings::Manager  (vorhanden, unverändert)
  → Mapper             (NEU, Fleißarbeit pro Feld; Vorbild: ConfigGroupLoader)
  → QaplaLlm::Actions::configureTournament/startTournament/…   (vorhanden)
  → GUI-Singletons → sichtbar
```

Die Aktionsschicht ist dafür bereits die richtige Grenze: sie kennt kein JSON und keine
Tool-Namen, nimmt Patch-Strukturen (`TournamentSettings`, `SprtSettings`, …) und meldet
verwertbare Fehler zurück.

Offen ist außerdem der **Eingabeweg**: `argv` beim GUI-Start ist der einfachste (die KI startet
die GUI mit Argumenten, der Nutzer schaut zu). Eine Kommandozeile *innerhalb* der GUI oder ein
beobachteter Ordner sind Varianten, aber jede zusätzliche Variante ist zusätzlicher Rand.

### Bewertung

**Umständlichkeit: mittel.** **Neuheitsgrad: mittel** — die Hälfte (Parser, Dateiformate) ist
geschenkt, die andere Hälfte (Mapper, eigener Dispatcher) ist neu und wächst mit jedem
Konfigurationsfeld mit. Der Satz „einfach nutzen, was da ist" stimmt für die Konfiguration,
nicht für die Ausführung.

**Zusätzlich unhandlich in der Benutzung:** Ein CLI-Aufruf ist ein einmaliger Anstoß. Für
„starte, beobachte, ändere Konzurrenz, stoppe" bräuchte es einen zweiten Kanal — und der ist
genau Punkt 3.

---

## 3. HTTP-Schnittstelle auf die vorhandenen GUI-Tools

### Idee

Die GUI läuft; die KI bedient sie über HTTP mit demselben Toolsatz, den heute der KI-Chat
benutzt. Der Nutzer sieht alles live.

### Warum das der kürzeste Weg ist

Die Grenze existiert bereits — `GuiToolRegistry` ist schon eine namens- und JSON-basierte
Fernsteuerung, nur ohne Netzwerk davor:

```cpp
std::vector<ToolSpec> exportToolSpecs() const;                          // Werkzeugliste
GuiToolResult callTool(const std::string& name, const std::string& argumentsJson);
void processQueue();   // führt im UI-Thread aus, einmal pro Frame
```

`callTool()` ist **bereits** dafür gebaut, aus einem fremden Thread aufgerufen zu werden: es
stellt in eine Queue ein, der UI-Thread arbeitet sie im Frame-Loop ab, der Aufrufer wartet mit
tool-eigenem Timeout. Ein HTTP-Server-Thread ist genau derselbe Fall wie der heutige
LLM-Worker-Thread. Der Adapter ist entsprechend dünn:

| Endpunkt | Abbildung |
|---|---|
| `GET /tools` | `exportToolSpecs()` als JSON |
| `POST /tools/{name}` | `callTool(name, body)` → `{success, content}` |
| `GET /status` | `Actions::runningActivitiesText()` + `activityStatus(...)` |

cpp-httplib kann Server; er ist bereits Submodul, CMake-Ziel und wird in den Tests schon als
Server betrieben. Keine neue Abhängigkeit, kein TLS (nur localhost).

### Die vom Nutzer genannte Einschränkung — halb zutreffend

- **Status gibt es schon als Text.** `get_status` (`Actions::activityStatus`) und
  `get_running_status` (`Actions::runningActivitiesText`) liefern vollständige Sätze, keine
  Widgets. Für „was läuft gerade, wie weit ist es" ist über HTTP nichts zu ergänzen.
- **Nur `show_result` ist bewusst ein Widget.** `GuiToolResult::renderWidget` zeichnet eine
  echte ImGui-Tabelle im Chat; `content` ist dabei absichtlich nur ein kurzer Begleitsatz.
  Über HTTP käme also nur dieser Satz an.
- **Die Textvariante gibt es teilweise schon**: `TableFormat::toText()` wird bereits benutzt,
  um die Ratingtabelle zu exportieren (`tournament-result-view.cpp:329`). Ein Tool bzw. ein
  Feld „Ergebnis als Text" ist damit eine Ergänzung im bekannten Muster, keine neue Mechanik.
  Der Umweg über das Log wäre möglich, aber deutlich unhandlicher.

### Varianten

1. **Eigenes, kleines HTTP/JSON-Protokoll** (Tabelle oben). Am geradlinigsten, ein Toolset auf
   Client-Seite bildet es ab.
2. **MCP über HTTP oder stdio, aber mit GUI-Handlern.** `McpServer` bringt Nachrichtenschleife,
   `tools/list`, `tools/call` und den Schema-Bauer schon mit; heute zeigt seine `callTool`-Weiche
   auf `AppRunner`. Sie stattdessen auf `GuiToolRegistry` zu legen, ergäbe eine Schnittstelle,
   die ohne eigenes Client-Toolset nutzbar ist. Etwas mehr Protokollrand, dafür Standard.

Variante 2 ist attraktiv, weil sie den bereits vorhandenen Schema-Export mitnimmt — aber sie
erbt auch den Anspruch, ein vollständiger MCP-Server zu sein. Variante 1 zuerst, Variante 2 als
mögliche Hülle darüber.

### Punkte, die zu klären sind

- **Bindung nur an `127.0.0.1`**, Port konfigurierbar (analog zur `[llmchat]`-Sektion),
  standardmäßig aus. Optional ein Token im Header.
- **Nur wenn die GUI rendert**: `processQueue()` läuft im Frame-Loop. Ein minimiertes oder
  blockiertes Fenster verzögert Tool-Aufrufe bis zum Timeout. Für den beabsichtigten Zweck
  („ich schaue zu") ist das kein Problem, sollte aber bekannt sein.
- **Dateidialoge**: Tools, die auf einen Menschen warten (`pickOpeningsFile` usw.), haben lange
  Timeouts. Über HTTP sind sie zulässig, aber der Aufrufer hängt so lange — besser explizite
  Pfade übergeben.
- **Gleichzeitige Bedienung** durch Mensch und KI serialisiert die Queue bereits; destruktive
  Aktionen (laufenden Test stoppen) bleiben wie im Chat bestätigungspflichtig oder zumindest
  klar quittiert.

### Bewertung

**Umständlichkeit: gering.** **Neuheitsgrad: sehr niedrig** — der weitaus kleinste neue Anteil
aller drei Punkte: ein Server-Thread und zwei bis drei Endpunkte auf eine Schnittstelle, die es
in genau dieser Form schon gibt. Zugleich der Punkt, der das eigentliche Ziel direkt trifft.

---

## Vergleich

| | Umständlichkeit | Neuheitsgrad | Wirklich neu daran |
|---|---|---|---|
| **1 CLOP im Chat** | gering | niedrig | Parameterliste variabler Länge; `TableData`→ImGui-Adapter |
| **2 CLI in der GUI** | mittel | mittel | Mapper `GroupInstance`→GUI und eigener Dispatcher; `AppRunner` ist unbrauchbar |
| **3 HTTP auf die GUI-Tools** | gering | sehr niedrig | Server-Thread, zwei Endpunkte, Ergebnistabelle auch als Text |

## Empfehlung zur Reihenfolge

1. **Punkt 3 zuerst.** Kleinster neuer Anteil, löst das genannte Ziel unmittelbar: die KI
   bedient die laufende GUI, der Nutzer sieht Partien und Tabellen live. Erste Ausbaustufe:
   `GET /tools`, `POST /tools/{name}`, `GET /status`, nur localhost, standardmäßig aus.
2. **Punkt 1 danach.** Bekanntes Muster, und CLOP profitiert am stärksten vom Zuschauen (lange
   Läufe, sich änderende Schätzwerte). Über Punkt 3 ist CLOP dann automatisch auch fernsteuerbar,
   weil es dieselbe Registry benutzt. SPSA fällt gleichartig ab.
3. **Punkt 2 nur bei Bedarf.** Sinnvoll, wenn Läufe aus vorbereiteten Konfigurationsdateien
   gestartet werden sollen. Dann aber bewusst als „Konfiguration übernehmen", nicht als
   „CLI ausführen" — und der Mapper ist ohnehin nützlich, sobald man `.qtour`/`.qsprt`-Dateien
   von außen anstoßen will.

## Offene Fragen

- Soll die HTTP-Schnittstelle im KI-Chat sichtbar sein (Aufrufe als Chat-Einträge protokolliert)?
  Dafür spricht die Nachvollziehbarkeit für den zuschauenden Nutzer, dagegen die Vermischung
  zweier Kanäle.
- Braucht CLOP eine eigene Ergebnisansicht außerhalb des Chats, oder reicht die Tabelle im
  Chat plus die Bretter?
- Bei Punkt 2: nur `argv` beim Start, oder auch eine Übergabe zur Laufzeit?
