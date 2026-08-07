# Finetuning-Datensatz für den AI-Chatbot

Trainingsdaten, um dem Chat-Modell die Fehler abzugewöhnen, die es im Betrieb macht —
vor allem: **behaupten statt handeln**. Das Modell antwortet regelmäßig „Das Turnier
startet jetzt", ohne je `start` aufgerufen zu haben.

## Warum es diesen Datensatz gibt

`LlmFineTuningWriter` (siehe `src/llm/llm-finetuning-writer.h`) hängt bei jedem
erfolgreichen Chat-Turn eine Zeile an `~/.qapla-chess-gui/finetuning.json`. Diese
Rohdatei ist **nicht** trainierbar: ihr Aufnahmekriterium ist „kein Tool hat einen Fehler
gemeldet", nicht „das Verhalten war richtig". Ein Turn ganz ohne Tool-Aufruf erfüllt das
trivial — also landen genau die Fehlschläge als scheinbar vorbildliche Beispiele darin.
Dazu verpackt `makeSyntheticReplyMessage()` (in `llm-chat-controller.cpp`) einen
Klartext-Fallback nachträglich in einen wohlgeformten `reply_to_user`-Aufruf: die *Form*
wird repariert, die *Halluzination* bleibt.

Deshalb ist jedes der 149 Records hier einzeln gelesen und die Antwortseite neu
geschrieben worden.

## Grundregel

> **Die Fehlersituation bleibt drin. Korrigiert wird nur die Zielantwort.**

Finetunen muss man das, was noch nicht funktioniert — ein Datensatz aus ausschließlich
gelungenen Turns trainiert das, was ohnehin klappt. Kein Record wurde gelöscht.

Bei SFT ist das gespeicherte Ziel aber immer das *korrekte* Verhalten; eine kaputte
Antwort als Ziel wäre eine Anweisung, den Fehler zu reproduzieren. Vorangestellter
Gesprächskontext darf dagegen die alte, falsche Antwort enthalten — Kontext ist Input,
nicht Trainingsziel. Dadurch bleibt genau das Muster trainierbar, an dem das Modell
scheitert: *User hakt nach, nachdem der Assistent gebremst hat → jetzt handeln.*

## Dateien

| Datei | Inhalt |
|---|---|
| `finetuning-001.json` | **Der Datensatz.** JSONL, ein `{"messages":[...]}` pro Zeile, OpenAI-Konvention. Generiert, nicht von Hand editieren. |
| `review-ledger-001.md` | Eine Zeile je Record: User-Nachricht, Tool-Kette, Begründung der Änderung. Zum Gegenlesen. |
| `original.json` | Kopie des Ist-Stands von `~/.qapla-chess-gui/finetuning.json` beim Start der Überarbeitung. Referenz und Kontrollgruppe. |
| `build.py` | Setzt die Records zusammen und schreibt Datensatz + Ledger. Dateinamen und Nummer stehen oben in der Datei. |
| `fixed_000.py` … `fixed_140.py` | **Die eigentliche Arbeit** — je zehn aus den Rohdaten korrigierte Records mit Tool-Kette, Antwort und Begründung. Hier wird editiert. |
| `added_*.py` | Neu geschriebene Records ohne Vorlage in den Rohdaten. Eigenes Präfix, damit die Herkunft sichtbar bleibt. |
| `verify.py` | Der Prüfer. |

### Nummerierung

Datensatz und Ledger sind durchnummeriert, weil weitere folgen werden. Wer einen neuen
aufsetzt, erhöht `DATASET`/`LEDGER` in `build.py` und den Default in `verify.py`; der alte
Datensatz bleibt unverändert liegen und bleibt damit reproduzierbar.

## Benutzung

```sh
python3 build.py                  # fixed_*/added_*  ->  finetuning-001.json + review-ledger-001.md
python3 verify.py                 # prüft den Datensatz                 -> erwartet 0
python3 verify.py original.json   # Gegentest gegen die Rohdaten        -> erwartet 149
```

Der Gegentest ist kein Beiwerk. Ein Prüfer, der „0 Beanstandungen" meldet, ohne je an
einer schlechten Datei gezeigt zu haben, dass er überhaupt anschlägt, beweist nichts —
genau daran ist der erste Aufräumversuch im August gescheitert (siehe
`docs/finetuning-data-cleanup-plan.md`; dessen Ergebnisteil erklärt die Datei für sauber,
geprüft wurde aber nur mit derselben Heuristik, die schon beim Suchen benutzt wurde).

## Was `verify.py` prüft

Der Prüfkatalog ist aus dem Systemprompt und den Tool-Schemata abgeleitet:

- Struktur: `system` zuerst, jeder Tool-Call von einem `tool`-Ergebnis gefolgt, kein
  Klartext neben `tool_calls`, genau ein Call pro Message
- nur existierende Tools, nur existierende Argumentnamen
- **Belegbarkeit**: jede Zahl und jeder Pfad in der Antwort muss in einem `tool`-Ergebnis,
  der User-Nachricht oder dem Kontext desselben Records vorkommen. Das ist die zentrale
  Regel — sie ist mechanisch entscheidbar und fängt genau die Halluzinationen
- keine leeren Antworten, keine Tool-Namen im Antworttext, keine Dialog-Narration
- keine kyrillischen/CJK-Zeichen (kamen real vor)
- **terminale Tools** beenden den Turn: danach darf keine Antwort mehr folgen

### Terminale Tools

`GuiToolResult::terminal` beendet den Turn ohne weitere Modell-Anfrage. Betroffen:

- `open_add_engine_dialog` — immer, auch bei Abbruch
- `configure_tournament` / `configure_sprt` / `configure_epd` mit einem
  `*_file_dialog`-Flag — `dialogOpened` wird unbedingt gesetzt, Abbruch zählt also auch
- `open_pgn_file` mit `source="dialog"` — ebenfalls, egal ob gewählt oder abgebrochen

Grund: der Datei-Picker nimmt den Fokus, und wenn das Modell zu Wort käme, wäre der
Dialog längst geschlossen. Es hat dort zuverlässig „bitte wählen Sie nun eine Datei"
geschrieben. 14 Records enden deshalb ohne Antwort.

## Konventionen im Datensatz

- **Sprache**: die Antwort spiegelt die Sprache des Users
- **Systemprompt**: in allen 149 Records byte-identisch. Ändert sich der Prompt in
  `chatbot-llm-chat.cpp`, ist der Datensatz entwertet und muss neu erzeugt werden
- **Tool-Ergebnisse** im heutigen Format, auch wo die Rohdaten noch ein älteres tragen
- **Kontext** wird als geflachtete `user`/`assistant`-Textnachrichten vorangestellt, exakt
  so, wie `LlmChatController` die History zur Laufzeit baut

## Stand

- **151 Records**: 149 aus den Rohdaten korrigiert (keiner gelöscht), davon 42 mit schweren
  Fehlern und 16 unverändert übernehmbar, plus 2 neu geschriebene (`added_pgn_dialog.py`)
- `verify.py`: **0** Beanstandungen gegen **149** bei den Rohdaten
- **Noch nicht übernommen**: `~/.qapla-chess-gui/finetuning.json` ist unangetastet

## Offene Punkte

1. Der Writer hängt weiter ungeprüfte Records an die Live-Datei an. Jede Session
   verwässert sie erneut — eine Kuratierung ist eine Momentaufnahme, keine Lösung.
2. Unterrepräsentierte Tools: `clear_result`, `close_application`, `open_add_engine_dialog`
   mit je einem Beispiel, `select_engines` und `select_epd_engines` mit je vieren.
