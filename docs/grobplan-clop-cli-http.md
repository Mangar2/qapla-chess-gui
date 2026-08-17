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

> Die Tabelle nennt nur die Größenordnung. Was die Schnittstelle fachlich können muss, wie die
> Tools zugeschnitten sein sollten, was MCP kostet und wo die heutigen Lücken sind, steht in der
> **Feinplanung** weiter unten.

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

## Offene Fragen zu Punkt 1 und 2

- Braucht CLOP eine eigene Ergebnisansicht außerhalb des Chats, oder reicht die Tabelle im
  Chat plus die Bretter?
- Bei Punkt 2: nur `argv` beim Start, oder auch eine Übergabe zur Laufzeit?

---
---

# Feinplanung: die Fernsteuerungsschnittstelle

Dieser Teil beantwortet vier Fragen: **wie wird die Fernsteuerung betrieben**, **wie sollten die
Tools geschnitten sein**, **was kostet MCP**, und **wo sind die Lücken**. Er macht bewusst keine
Umsetzungsvorgaben — keine Pfade, keine JSON-Formate, keine Klassennamen. Er sagt, *was* die
Schnittstelle können muss und *woran* sie heute noch scheitern würde.

## F.1 Betriebsart „Fernsteuerung"

### Start

Die Fernsteuerung wird über einen **Kommandozeilenschalter beim Start der GUI** eingeschaltet,
nicht über ein Tool und nicht über einen Knopf im Chat. Grund: Wer die GUI startet, entscheidet
damit auch, wofür sie in dieser Sitzung da ist. Eine Fernsteuerung, die sich selbst
einschalten kann, ist ein Sicherheitsproblem ohne Nutzen.

Kleiner Vorbehalt: `main()` in `src/qapla-chess-gui.cpp:435` nimmt heute **keine Argumente**
entgegen. Der Einstiegspunkt muss also ohnehin angefasst werden — was gleichzeitig die
Grundlage für Punkt 2 (CLI) wäre, falls der je kommt.

### Das Fernsteuerungsfenster

Ein **eigenes Verlaufsfenster**, das dem KI-Chat ähnelt, aber nicht dasselbe ist. Es zeigt
lückenlos, was von außen ausgelöst wurde: jeder Aufruf mit Zeitpunkt, Name, Argumenten und
Ergebnis, in derselben Darstellung, die der KI-Chat für Tool-Einträge schon benutzt — inklusive
der Tabellen (siehe F.6). Das ist der Kern des ganzen Vorhabens: **zuschauen können.**

Solange die Fernsteuerung läuft:

- sind die **übrigen Chat-Threads ausgeblendet** (klassische Abläufe und KI-Chat). Es gibt genau
  einen Steuerkanal, und der ist von außen.
- gibt es einen Knopf **„Fernsteuerung beenden"**. Er schließt den Kanal und gibt die GUI zur
  normalen Bedienung frei. Das ist eine **Einbahnstraße** — ein Zurückschalten in die
  Fernsteuerung ist vorerst nicht vorgesehen (ein Hin und Her verdoppelt die Zustände, ohne dass
  klar ist, wozu).
- bleibt die GUI **vollständig normal bedienbar**: Bretter ansehen, Tabs links umschalten,
  Turnier- und SPRT-Ansicht mit ihren laufenden Zahlen.

### Parallelbedienung

Der Nutzer darf gleichzeitig eingreifen. Wenn die KI ein Turnier startet und der Nutzer es
stoppt, ist das erlaubt und wird **nicht verhindert**. Zwei Gründe: Sperren würden genau das
kaputt machen, was die GUI hier voraushat (der Mensch bleibt Herr über seine Anwendung), und
die Schnittstelle muss ohnehin damit klarkommen, dass sich der Zustand zwischen zwei Aufrufen
ändert — siehe F.4, „Zustand statt Gedächtnis". Ein Eingriff von Hand ist für die Schnittstelle
nichts anderes als ein Turnier, das von selbst zu Ende ging.

Wichtig ist nur, dass die KI **merkt**, dass etwas passiert ist. Genau dafür sind die
Fertigmeldung (F.5) und der Grundsatz „jede Antwort trägt den Zustand mit" (F.4) da.

### Beenden

„Fernsteuerung beenden" **stoppt keine laufenden Läufe.** Es schließt nur den Kanal. Ein
laufendes Turnier läuft weiter und ist danach von Hand zu bedienen. Offene Aufrufe von außen
bekommen eine klare Absage statt einer Zeitüberschreitung.

## F.2 Toolstruktur: pro Aktion oder pro Thema?

### Was heute wirklich da ist

Die Struktur ist bereits ein **Mischform**, nicht „ein Tool pro Aktion":

> Beschreibt den Stand **vor** dem Aufräumen aus Stufe 0 (F.8); die Bewertung führte zu genau
> diesem Aufräumen.

| Achse | Tools |
|---|---|
| Pro Thema (Konfiguration) | `configure_tournament`, `configure_sprt`, `configure_epd`, dazu `select_engines`, `select_sprt_engines`, `select_epd_engines` |
| Pro Verb, Thema als Parameter | `start`, `stop`, `get_status`, `clear_result`, `show_result` — alle mit `type` = tournament/sprt/epd |
| Übergreifend | `get_running_status` |
| Anwendung / Katalog | `list_installed_engines`, `open_add_engine_dialog`, `open_pgn_file`, `close_application` |

16 Tools. Die fünf Verb-Tools sind **schon** eine Zusammenlegung — der Kommentar in
`gui-tools.h:58-65` sagt genau warum: „ein kleines Modell trifft aus fünf Tools plus einem
Enum weit zuverlässiger die richtige Wahl als aus fünfzehn Namen." Die Erfahrung, die zu
diesem Vorschlag geführt hat, ist im Code also bereits verarbeitet — nur nicht auf der
Themenachse, sondern auf der Verbachse.

### Was gegen die reine Themenlösung spricht

Ein Tool pro Thema (`tournament(action, …)`, `sprt(action, …)`, …) reduziert 16 Namen auf etwa
5. Es verlagert das Problem aber, statt es zu lösen:

- **Das Schema wird zur Vereinigungsmenge.** `tournament` müsste die Felder von Konfigurieren,
  Starten, Stoppen, Löschen und Anzeigen gemeinsam führen. Alle wären optional, weil jedes nur
  für manche Aktionen gilt. Damit fällt die Schemaprüfung als Schutz weg: heute lehnt der Mapper
  einen fehlenden Pflichtparameter ab (`llm-tool-api.h`), künftig wäre „`action=stop` mit
  `games=10`" schematisch einwandfrei und fachlich Unsinn.
- **Die Beschreibung wird nicht kürzer, sondern länger.** Sie muss zusätzlich erklären, welches
  Feld zu welcher Aktion gehört. Die Token-Ersparnis ist gering (siehe F.3: die Namen sind
  wenige Prozent, der Beschreibungstext ist alles).
- **Die zerstörende Aktion wird zum Parameterwert.** `stop` als Toolname ist auf einen Blick
  erkennbar — im Verlauf, in einem Freigabedialog, in einer Auswertung. `tournament` mit
  `action="stop"` ist es nicht. Für eine Fernsteuerung, bei der ein Mensch zuschaut, ist das ein
  echter Verlust.

### Wo die Verwechslung tatsächlich sitzt

Nicht bei `start`/`stop` — sondern bei den **drei Auskunftstools**: `get_status`,
`get_running_status`, `show_result`. Ihre Beschreibungen sind heute die längsten überhaupt und
bestehen zum großen Teil daraus, sich gegeneinander abzugrenzen („use this instead of that",
„this is only needed for a pure check that changes nothing"). Das ist das Eingeständnis, dass
der Schnitt nicht selbsterklärend ist.

### Vorschlag: der Mittelweg

Nicht die Achse wechseln, sondern dort zusammenlegen, wo die Fehlgriffe passieren:

| Maßnahme | Wirkung |
|---|---|
| Die drei Auskunftstools zu **einem** zusammenführen: ohne `type` die Übersicht über alle drei, mit `type` alles über eine — Konfiguration, Laufzustand **und** Ergebnis | 3 → 1; die längsten Abgrenzungstexte entfallen; löst gleichzeitig das Widget/Text-Problem (F.6) |
| `select_*_engines` in `configure_*` aufgehen lassen — die Engineauswahl ist ein Konfigurationsfeld wie jedes andere (sie hat nur eine eigene Sperrregel, und die steht ohnehin schon im Konfigurationstool) | 3 → 0 |
| `start`, `stop`, `clear_result` unverändert lassen | Verben bleiben Namen |

Ergebnis: **16 → 11 Tools**, ohne Vereinigungsschema, ohne Verlust an Prüfbarkeit, mit
spürbar weniger Beschreibungstext. Wenn CLOP dazukommt, wächst nur der `type`-Enum, nicht die
Toolzahl.

### Und: erst messen, dann schneiden

Beides ist bereits vorhanden und beantwortet die Frage besser als eine Meinung:

- `LlmChatLogger::logSystemPromptAndToolsOnce()` schreibt Systemprompt **und** die vollständige
  Tools-JSON einmal pro Sitzung mit — daraus ist die tatsächliche Größe exakt bestimmbar.
- `LlmChatLogger` protokolliert jeden Aufruf, `llm-finetuning-writer` sammelt bereits
  Trainingsdaten.

Damit lässt sich zählen, *welche* Tools tatsächlich verwechselt werden, statt es zu vermuten.
Empfehlung: die Auskunftstools zusammenlegen (das ist unabhängig von der Messung richtig, weil
es die Widget/Text-Frage mitlöst), den Rest anhand der Logs entscheiden.

## F.3 MCP: was es kostet, was es bringt

### Ausgangslage

Der qapla-engine-tester hat einen funktionierenden MCP-Server (`src/mcp/`, über stdio). Er ist
aus demselben Grund nicht nutzbar wie die CLI: seine `callTool`-Weiche führt über `AppRunner`
aus und steuert die GUI nicht. Wiederverwendbar sind die **Form** und der Schema-Bauer
(`mcp-schema-builder`, `mcp-converter`), nicht die Handler.

### Wie viele Token kostet MCP zusätzlich?

**Nahezu keine.** Das ist die wichtigste Antwort dieses Abschnitts, und sie ist leicht
zu übersehen, weil MCP nach „mehr Protokoll" aussieht.

Im Kontextfenster des Modells landet in beiden Fällen dasselbe: Name, Beschreibung und
Parameterschema jedes Tools. Der JSON-RPC-Rahmen (`jsonrpc`, `id`, `method`), der
`initialize`-Handschlag und die Ergebnishülle laufen zwischen Client und Server über die
Leitung — sie erreichen das Modell nicht. Der Kostentreiber ist unser eigener
Beschreibungstext, nicht das Protokoll.

Größenordnung, gemessen an den heutigen Tool-Dateien:

| Posten | Umfang |
|---|---|
| Reiner Text in `src/llm/tools/` (Beschreibungen, Parametertexte, Enumwerte) | ~16.800 Zeichen (vor dem Aufräumen aus Schritt 0: ~17.500) |
| Zuzüglich JSON-Struktur (Schemarahmen, Eigenschaftsobjekte) | grob 22.000–23.000 Zeichen |
| **Ergibt** | **grob 5.000–6.000 Token für die vollständige Werkzeugliste** |
| Aufschlag durch MCP: Namenspräfixe des Clients (z. B. `qapla__start`), je nach Client | ~50–150 Token gesamt |
| Aufschlag durch MCP: `instructions` aus `initialize`, sofern gesetzt | so viel, wie man hineinschreibt — also 0, wenn man es leer lässt |
| Aufschlag durch MCP: Ergebnishülle pro Aufruf | wenige Token |

Also **deutlich unter 5 % Aufschlag**, realistisch unter 2 %. Der exakte Wert ist nicht zu
schätzen, sondern abzulesen — siehe die Tools-JSON im Log (F.2).

Zwei Kostenfallen, die man sich mit MCP aber leicht *einhandelt*, weil das Protokoll sie anbietet:

- **Resources und Prompts.** Beide landen zusätzlich im Kontext. Für unseren Zweck brauchen wir
  keine. Bewusst weglassen.
- **Ausführliche Fehlerobjekte.** Ein Fehler, der als Prosa zurückkommt, ist kürzer *und* für
  das Modell brauchbarer als ein strukturierter Fehlercode, den es doch nur in Prosa übersetzt.
  Die heutigen Ergebnistexte sind hier schon richtig gebaut.

### Der lokale kleine Modell-Pfad bleibt unangetastet

LM Studio spricht OpenAI-Funktionsaufrufe, nicht MCP. Für das kleine Modell im KI-Chat ändert
sich also **gar nichts** — es bekommt weiter dieselbe Liste im selben Format aus derselben
Registry. MCP ist ausschließlich die Hülle nach außen.

Das ist der eigentliche Grund, warum MCP hier billig ist: **eine Quelle, zwei Hüllen.** Tools
werden weiterhin einmal als Daten deklariert (`src/llm/tools/`); die eine Hülle rendert daraus
das OpenAI-Format für LM Studio, die andere das MCP-Format für außen. Ein zweiter Handler-Satz
entsteht nicht, und die beiden Hüllen können nicht auseinanderlaufen, weil es nur eine
Deklaration gibt.

### Was der Cache verlangt — als verbindliche Regeln

Der Vorteil, den der Nutzer beschreibt (immer dieselbe vollständige Schnittstelle, dadurch
stabiler Prompt-Präfix und schnelle Antworten), ist mit MCP genauso erreichbar — aber nur, wenn
man ihn nicht selbst zerstört. Deshalb als Festlegung, für **beide** Hüllen:

1. **Die Werkzeugliste ist statisch.** Sie hängt nie vom Zustand ab. Kein „`stop` erscheint nur,
   wenn etwas läuft". Zustand gehört ins Ergebnis, nie in die Toolauswahl. (Heute schon so:
   `registerGuiTools()` registriert unbedingt.)
2. **Die Reihenfolge ist stabil.** Registrierung in fester Folge, keine Umsortierung, keine
   Aufnahme über ungeordnete Behälter.
3. **Keine `listChanged`-Benachrichtigungen.** Sie sind genau das Gegenteil eines stabilen
   Präfixes.
4. **Kein zweistufiges Entdecken.** Nichts, wofür das Modell erst ein Tool aufrufen muss, um zu
   erfahren, wie es ein anderes aufruft. Alles Aufrufwissen steht in der Beschreibung. (Heute
   schon so und ausdrücklich so gemeint — vgl. den Hinweis bei `select_engines`, dass
   `list_installed_engines` vorher *nicht* nötig ist.)
5. **Änderungen an Beschreibungen sind Cache-Ereignisse.** Sie sind trotzdem erlaubt und
   erwünscht — aber gebündelt, nicht laufend im Betrieb.

### Empfehlung

MCP ja, aber als **zweiter Schritt**. Zuerst die schlanke HTTP-Fassung, weil sie ohne
Protokollrand auskommt und die Fragen aus F.4/F.5 zuerst geklärt werden müssen — die sind
protokollunabhängig und der eigentliche Inhalt. Steht das, ist MCP darüber eine Hülle und keine
neue Schnittstelle.

## F.4 Lücken und Härtung

Grundsatz vorweg, in vier Sätzen — sie beschreiben, was die vorhandenen Aktionen bereits
richtig machen, und sind deshalb als Regel für alles Neue formuliert:

- **Ergebnis statt Absicht.** Eine Antwort beschreibt, was *jetzt gilt*, nicht was gleich
  passieren wird. Wo das nicht geht, sagt sie ausdrücklich, dass es noch nicht so weit ist.
- **„Nichts geändert" ist eine Antwort.** Ein Aufruf, der nichts bewirkt hat, muss das sagen —
  sonst wird auf eine Änderung gewartet, die nie kommt.
- **Der nächste Schritt steht im Fehlertext.** Nicht nur was falsch ist, sondern was zu tun ist.
- **Zustand statt Gedächtnis.** Die Antwort trägt den relevanten Zustand mit, damit die
  Gegenseite ihn nicht aus früheren Antworten rekonstruieren muss — der Nutzer kann ihn
  zwischendurch geändert haben.

### Die Stopp-Lücke — weitgehend geschlossen, aber nicht überall geprüft

Das vom Nutzer beobachtete Verhalten ist im Code inzwischen adressiert, und zwar genau in der
richtigen Weise:

- `stopTournament(Abrupt)` ruft `stopPoolAbruptlyAndWait()` und kehrt erst zurück, wenn die
  Partien wirklich weg sind. Der Kommentar dazu benennt den alten Fehler beim Namen: „Reporting
  ‚is being aborted' and returning early is what produced the retry loops."
  (`gui-action-tournament.cpp:529`). SPRT und EPD haben dieselbe Behandlung
  (`gui-action-sprt.cpp:556`, `gui-action-epd.cpp:410`).
- Der schonende Stopp sagt ausdrücklich „Not done yet".
- Ein zweiter Stoppbefehl während des Stoppens antwortet „Already stopping. Nothing changed."
- Die Zustände unterscheiden `Running`, `GracefulStopping` und `Stopping`, und `start` gibt je
  Zustand einen anderen, jeweils richtigen Rat („Wait" statt „Stop it first").

**Was bleibt:** Diese Sorgfalt ist an drei Stellen parallel implementiert. Sobald CLOP dazukommt,
gilt sie dort erneut — deshalb sollte das Muster (Zustände, Sperrregeln, Wortlaut der
Verweigerungen) als **gemeinsame Regel** festgehalten werden, nicht als drei Kopien, die je
einzeln richtig sein müssen. Der Prüfpunkt ist konkret: **Wo endet ein Aufruf, während sein
Effekt noch läuft?** Heute nur beim schonenden Stopp — und dort ausgesprochen.

### Die verbleibenden Lücken

| Lücke | Warum sie über HTTP stärker wiegt als im Chat | Richtung |
|---|---|---|
| **Keine Fertigmeldung.** Es gibt keine Möglichkeit zu warten, bis ein Lauf zu Ende ist. | Im Chat sitzt ein Mensch davor und sieht es. Von außen bleibt nur Pollen — teuer und ungenau. | Eigener Abschnitt F.5 |
| **Ergebnisse gibt es nur als Bild.** `show_result` liefert bewusst ein Widget; `content` ist nur ein Begleitsatz. Die Beschreibung sagt sogar, es sei die *einzige* Quelle für Ergebnisse. | Über die Leitung käme damit nie ein Ergebnis an. Das ist die schwerste inhaltliche Lücke. | Ergebnis zusätzlich als Text (Grundlage vorhanden: `TableFormat::toText()`), zusammengelegt mit der Auskunft (F.2) |
| **Kein Laufkennzeichen.** „Das SPRT ist fertig" — welches? Nach Stopp, Löschen und Neustart ist das von außen nicht unterscheidbar. | Der Chat hat den Gesprächsverlauf als Kontext, die Fernsteuerung nicht. Nutzer können dazwischenfunken. | Jeder Lauf bekommt eine Kennung, die in Status, Ergebnis und Fertigmeldung erscheint |
| **Kein Änderungszähler.** Man kann nicht billig fragen „hat sich seit meinem letzten Blick etwas geändert". | Führt zu großen Statusabfragen im Sekundentakt. | Ein monoton wachsender Zähler je Betriebsart, der bei jeder Zustandsänderung steigt |
| **Wiederholte Aufrufe nach Zeitüberschreitung.** Läuft ein Aufruf in den Timeout der Registry, weiß der Aufrufer nicht, ob die Aktion trotzdem lief. | Im Chat wiederholt ein Mensch bewusst; ein Werkzeug wiederholt automatisch. | Zerstörende Aufrufe (`clear_result`, `stop`) müssen wiederholbar sein, ohne zusätzlichen Schaden — heute weitgehend gegeben („Already stopping. Nothing changed."), aber ungeprüft |
| **Dateidialoge.** `open_add_engine_dialog`, `pgn_file_dialog`, `openings_file_dialog` blockieren auf einen Menschen, mit sehr langen Zeitgrenzen. | Von außen aufgerufen hängt der Aufrufer minutenlang an einem Fenster, das er nicht sieht. | Über die Fernsteuerung nicht anbieten, oder nur mit sofortiger Rückmeldung „ein Dialog ist offen, der Nutzer ist am Zug" |
| **`close_application`.** | Beendet genau das, was man beobachten wollte, und den Kanal gleich mit. | Über die Fernsteuerung nicht anbieten |
| **`endsTurn` hat außen keine Bedeutung.** Das Feld steuert den Agenten-Loop des lokalen Chats. | Über HTTP gibt es keinen Turn, den man beenden könnte. | Als das kennzeichnen, was es ist: eine Eigenschaft der lokalen Hülle, nicht der Aktion |
| **CLOP und SPSA fehlen.** | — | Kommt mit Punkt 1; die Schnittstelle wächst dann nur um Enumwerte |

### Verständlichkeit

Zwei Dinge, die die Schnittstelle unabhängig von allem anderen leichter benutzbar machen:

- **Ein gemeinsames Vokabular für Zustände.** Heute sind die Zustände je Betriebsart eigene
  Enums mit teils abweichenden Namen (`EpdData::State::Gracefully` gegenüber
  `GracefulStopping` bei Turnier und SPRT). Nach außen sollte es **einen** Satz Zustandsnamen
  geben, für alle Betriebsarten gleich, CLOP eingeschlossen.
- **Sperrregeln als Zustand, nicht nur als Fehlermeldung.** Dass Einstellungen während eines
  Laufs gesperrt sind, erfährt man heute erst, wenn man es versucht. Wenn die Auskunft mitliefert,
  was gerade änderbar ist, entfällt eine ganze Klasse von Fehlversuchen.

## F.5 Fertigmeldung: „das SPRT ist durch"

Das ist die einzige wirklich fehlende Fähigkeit — alles andere in F.4 ist Schärfen von
Vorhandenem.

**Was gebraucht wird:** ein Aufruf, der **wartet**, statt sofort zu antworten. Er kehrt zurück,
wenn sich der Zustand der beobachteten Betriebsart ändert — Lauf beendet, vom Nutzer gestoppt,
abgebrochen — oder wenn eine mitgegebene Zeitgrenze abläuft (dann mit der Aussage „nichts
passiert, es läuft noch"). Er verändert nichts.

**Warum genau das die richtige Form ist:** Es bildet das Verhalten ab, das der Nutzer von der
CLI kennt und schätzt. Dort startet die KI einen Hintergrundprozess; endet er, wird sie
aktiviert. Ein wartender Aufruf leistet dasselbe, ohne dass die GUI irgendetwas „pushen" muss:
Die Gegenseite wartet in einem Hintergrundprozess auf die Antwort, und dessen Ende ist das
Aktivierungssignal. Die GUI braucht dafür weder eine Rückrufadresse noch Kenntnis davon, wer
gerade zuhört.

**Was die Meldung tragen muss:**

- Wodurch das Warten endete: fertig, vom Nutzer gestoppt, abgebrochen, Zeitgrenze.
- Die Laufkennung — damit klar ist, *welcher* Lauf gemeint war.
- Genug Zustand, um ohne weiteren Aufruf weiterarbeiten zu können (vgl. „Zustand statt
  Gedächtnis"). Ein SPRT, das mit einer Entscheidung endet, sollte die Entscheidung gleich
  mitbringen.

**Randfälle, die mitgeplant gehören:**

- Mehrere Warteaufrufe gleichzeitig (etwa auf Turnier und SPRT) müssen sich nicht behindern.
- „Fernsteuerung beenden" und das Schließen der GUI beenden alle Warteaufrufe **mit Begründung**,
  nicht durch Verstummen.
- Die Zeitgrenze muss vom Aufrufer kommen und darf nicht so großzügig sein, dass ein hängender
  Aufruf nicht mehr von einem wartenden zu unterscheiden ist.
- Wer wartet, muss auch **ohne** Warten an denselben Zustand kommen — sonst wird der Warteaufruf
  zum Nadelöhr.

**Bewusst nicht zuerst:** ein Ereignisstrom (SSE) oder ein Rückruf an die KI. Beides ist
mächtiger, aber beides verlangt auf der Gegenseite eine dauerhaft lauschende Instanz. Der
wartende Aufruf kommt mit dem aus, was ohnehin da ist. Ein Ereignisstrom ist später ergänzbar,
wenn sich zeigt, dass Zwischenstände (Elo-Verlauf, CLOP-Schätzwerte) live gebraucht werden.

## F.6 Wer sieht was: Tabelle auf den Schirm, Text auf die Leitung

Der scheinbare Widerspruch — die GUI zeigt Ergebnisse als lebende Tabelle, die Leitung kann nur
Text transportieren — löst sich, wenn man ihn nicht als Entweder-oder behandelt.

Ein Aufruf erzeugt **beides aus einer Quelle**: die Tabelle wird im Fernsteuerungsfenster
gezeichnet (der Mensch sieht sie, sie aktualisiert sich weiter), und derselbe Inhalt geht als
Text über die Leitung. Der Mechanismus für die Anzeige existiert bereits
(`GuiToolResult::renderWidget`, wird jedes Frame neu gezeichnet), der für den Text zur Hälfte
(`TableFormat::toText()`).

Damit fällt auch die heutige Ausnahme weg, dass `show_result` etwas anderes tut als alle
übrigen Tools — und zusammen mit der Zusammenlegung aus F.2 verschwindet die längste
Abgrenzungsbeschreibung der ganzen Schnittstelle.

## F.7 Sicherheit und Betrieb

- Nur an `127.0.0.1` binden, Port einstellbar, **standardmäßig aus** — an nur über den
  Kommandozeilenschalter. Ein einfaches Merkmal zur Zuordnung (Token) genügt für den Fall, dass
  auf dem Rechner mehrere Instanzen laufen; TLS ist nicht das Thema.
- Aufrufe werden erst ausgeführt, **wenn die GUI rendert** (die Warteschlange wird im Frame-Lauf
  abgearbeitet). Ein minimiertes oder hängendes Fenster verzögert also alles bis zur Zeitgrenze.
  Für den Zweck unproblematisch, aber es gehört in die Dokumentation — und es ist ein Argument
  dafür, dass die Fernsteuerung ein sichtbares Fenster hat.
- Der Verlauf im Fernsteuerungsfenster ist **die** Nachvollziehbarkeit. Er sollte
  mitgeschrieben werden wie der KI-Chat heute (`LlmChatLogger`), damit sich hinterher klären
  lässt, wer was ausgelöst hat — die KI oder der Nutzer.

## F.8 Abnahmefähige Ausbaustufen

Jede Stufe endet mit etwas, das man sehen kann.

**Stufe 0 — Aufräumen (erledigt).** Vor jeder Fernsteuerung, weil sie sonst eine Schnittstelle
nach außen führt, die man gleich danach wieder ändert. Wirkt auf den KI-Chat genauso, denn es
gibt nur eine Deklaration.

- **16 → 11 Tools.** `select_engines`/`select_sprt_engines`/`select_epd_engines` sind in
  `configure_tournament`/`configure_sprt`/`configure_epd` aufgegangen (die Engines sind ein Feld
  wie jedes andere, und beim Turnier und bei EPD gilt: lässt sich ein Name nicht auflösen, wird
  gar nichts angewendet). `get_status`/`get_running_status`/`show_result` sind ein `get_status`:
  ohne `type` die Übersicht über alle drei, mit `type` Konfiguration, Laufzustand **und**
  Ergebnistabelle in einer Antwort.
- **Ein Zustandsvokabular für alle.** `RunState` plus die Formulierungen liegen einmal in
  `src/llm/actions/gui-action-types.*`; Turnier, SPRT und EPD bilden nur noch ihre eigenen Enums
  darauf ab. Vorher waren es drei handgeschriebene Wortlaute, die beim Schweren übereinstimmten
  („ein schonend stoppender Lauf ist nicht ‚running'") und beim Leichten auseinanderliefen
  („stopping abruptly" gegen „being aborted" für denselben Zustand). CLOP liefert später nur die
  Abbildung.
- **Sperrregeln als Zustand.** Die Statusantwort sagt jetzt selbst, was gerade änderbar ist,
  statt es erst beim abgelehnten Versuch zu verraten. Einstellungen und Engineauswahl teilen sich
  eine Sperre und einen Wortlaut.
- **Vollständigere Antworten.** `clear_result` hängt wie `start`/`stop` die
  betriebsartübergreifende Zusammenfassung an; `configure_epd` endet wie die beiden anderen
  Konfigurationstools mit dem vollen Status statt mit einer Liste geänderter Felder.
- **Weniger Text, nicht mehr.** Trotz der neuen Engine-Parameter ist die Schnittstelle um rund
  700 Zeichen kleiner: die weggefallenen Abgrenzungstexte („nimm dieses statt jenes") waren die
  längsten überhaupt.

*Fertig heißt:* Die GUI und die Tests laufen gegen die neuen elf Tools; der KI-Chat benutzt
dieselben.

**Stufe 1 — die GUI lässt sich von außen bedienen und man sieht dabei zu.**
Kommandozeilenschalter, Fernsteuerungsfenster mit Verlauf, übrige Threads ausgeblendet,
„Fernsteuerung beenden"-Knopf, Werkzeugliste und Aufruf über HTTP, Statusabfrage.
*Fertig heißt:* Von außen ein Turnier konfigurieren und starten; die Partien laufen sichtbar auf
den Brettern, jeder Aufruf steht im Verlauf, der Knopf gibt die GUI wieder frei.

**Stufe 2 — Ergebnisse kommen auch außen an (erledigt).**
Auskunftstools zusammengelegt und Zustandsnamen vereinheitlicht waren schon in Stufe 0 dabei;
dazugekommen sind:

- **Ergebnisse als Text, aus derselben Tabelle.** `ImGuiTable::toText()` liest die Zeilen aus dem
  Steuerelement, das die GUI ohnehin zeichnet — Turnierstand, SPRT-Entscheidung samt Duellstand,
  EPD-Positionen. Kein zweiter Datenpfad, also kein Weg, auf dem Schirm und Leitung sich über
  eine Zahl uneinig werden können. Lange Ergebnislisten werden nach 40 Zeilen mit einer Angabe
  abgeschnitten, wie viele fehlen.
- **Dabei eine Falle gefunden:** diese Tabellen sind ein Zeichen-Zwischenspeicher, den `pollData()`
  nur während eines Laufs füllt. Ein fertiger Test, der sichtbar auf dem Schirm stand, hätte
  „keine Ergebnisse" geliefert — genau im wichtigsten Moment. `resultsAsText()` füllt deshalb
  zuerst neu und ist dafür nicht mehr `const`.
- **`CallOrigin`.** Die Registry kennt jetzt „aus dem Fenster" und „von außen". Ein Tool oder ein
  Parameter kann `localOnly` sein: aus der veröffentlichten Werkzeugliste entfernt und abgelehnt,
  falls doch übergeben. Betroffen sind `open_add_engine_dialog`, `open_pgn_file` und
  `close_application` sowie die drei Dateidialog-Schalter. Abgelehnt, nicht stillschweigend
  ignoriert: wer einen Dateidialog wollte, will eine gewählte Datei, und der Rest des Patches
  dürfte nicht als Erfolg zurückgemeldet werden.
- Damit sieht ein Fernaufrufer 8 der 11 Tools.

*Fertig heißt:* Von außen ist der Turnierstand abfragbar, ohne auf den Bildschirm zu sehen — und
dieselbe Abfrage zeichnet im Fenster die Tabelle.

**Stufe 3 — die Fertigmeldung (erledigt).**

- **`GET /wait?type=…&timeout=…`** antwortet nicht, bis der Lauf steht. Der Grund kommt mit:
  `finished` (eigener Abschluss), `stopped` (jemand hat beendet), `timeout`, `closed` (Kanal zu),
  `not_running` (es lief nichts). Dazu die **Laufkennung** und der **Änderungszähler**, und der
  volle Status samt Ergebnistabelle — wer geweckt wird, hat die Entscheidung schon in der Hand
  und braucht keinen zweiten Aufruf.
- **`ActivityWatch`** hält die Buchführung: bewusst frei vom GUI-Stapel und von außen mit
  einfachen Werten gefüttert, damit der wirklich nebenläufige Teil ohne Fenster testbar ist.
  Gefüttert wird aus dem Frame-Takt — der bemerkt jedes Ende, egal auf welchem Weg, auch das
  Stoppen von Hand.
- **Ein Rennen dabei gefunden.** `start` kehrt zurück, bevor je ein Frame gezeichnet wurde. „Lauf
  starten, dann warten" — also genau die Abfolge, für die der Warteaufruf existiert — bekam
  deshalb `not_running` über einen Lauf, der gerade begonnen hatte. Die drei Aktionen melden ihren
  Zustand jetzt zusätzlich sofort selbst.
- **Was ein wartender Aufruf nicht tut:** er belegt eine Verbindung und einen Server-Thread, sonst
  nichts. Der UI-Thread bleibt unberührt, das Fenster so bedienbar wie zuvor, und Warteaufrufe auf
  die drei Betriebsarten behindern sich nicht gegenseitig.

*Fertig heißt:* Ein SPRT wird von außen gestartet; die Gegenseite wartet im Hintergrund und wird
mit Entscheidung und Grund geweckt, sobald es durch ist — auch wenn der Nutzer es von Hand
gestoppt hat.

**Stufe 4 — MCP als Hülle.**
Dieselbe Registry, MCP-Format nach außen, keine Resources, keine Prompts, statische
Werkzeugliste.
*Fertig heißt:* Ein MCP-Client sieht dieselben Werkzeuge und steuert dieselbe GUI; der lokale
KI-Chat ist unverändert.

**Stufe 5 — CLOP zieht ein.**
Nur noch Enumwerte und ein Konfigurationstool, sonst nichts Neues an der Schnittstelle.

## F.9 Entschiedenes und Offenes

**Entschieden (in Stufe 0 umgesetzt):**

- **Toolschnitt:** der Mittelweg aus F.2, 16 → 11. Kein Wechsel auf ein Tool pro Thema.
- **Er wirkt auch auf den KI-Chat.** Eine Quelle, zwei Hüllen ist nur so viel wert, wie beide
  Seiten dieselbe Schnittstelle sehen. Folge: die bisher gesammelten Feinabstimmungsdaten
  veralten in Teilen.
- **Zustand in der Antwort:** die zustandsändernden Tools (`start`, `stop`, `clear_result`)
  hängen die betriebsartübergreifende Zusammenfassung an, die Konfigurationstools enden mit dem
  vollen Status ihrer eigenen Betriebsart. Nicht jede Antwort trägt alles.
- **Das Fernsteuerungsfenster zeigt nicht, was der Nutzer selbst tut.** Es ist der Verlauf des
  Fernsteuerkanals, keine allgemeine Ereignisliste. Dass ein Eingriff von Hand die KI überraschen
  kann, wird stattdessen dort aufgefangen, wo es zählt: jede Antwort trägt den aktuellen Zustand
  mit, und die Fertigmeldung (F.5) nennt ausdrücklich, wenn ein Lauf vom Nutzer beendet wurde.

**Offen:**

- **Welche Tools werden tatsächlich verwechselt?** `LlmChatLogger` schreibt jeden Aufruf mit; nach
  ein paar Sitzungen mit einem kleinen Modell lässt sich das zählen, statt es zu vermuten. Erst
  danach lohnt ein weiterer Schnitt.
- **Braucht `get_status` mit `type` eine kurze Form?** Heute liefert es immer alles, auch wenn nur
  nach einem Feld gefragt war. Bewusst so — ein falsch gewähltes Tool ist ein Fehler, zu viel
  Information nicht —, aber bei sehr kleinem Kontextfenster könnte das kippen.
