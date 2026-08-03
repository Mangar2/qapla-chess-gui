# Plan: Aufräumen von finetuning.json

## Ausgangslage

`~/.qapla-chess-gui/finetuning.json` wird seit Beginn der LLM-Chat-Entwicklung von
`LlmFineTuningWriter` befüllt (siehe `src/llm/llm-finetuning-writer.h/.cpp`) — eine
Zeile pro erfolgreich abgeschlossenem Chat-Turn, JSONL-Format, jede Zeile ein
vollständiges `{"messages":[...]}`-Objekt im OpenAI-Wire-Format
(`system, user, assistant(+tool_calls), tool, ..., assistant(reply_to_user)`).
Nur Turns, bei denen jeder echte Tool-Aufruf erfolgreich war, werden überhaupt
aufgezeichnet; ein finaler Klartext-Fallback statt `reply_to_user` wird beim
Schreiben bereits automatisch in die korrekte `reply_to_user`-Form korrigiert (siehe
`makeSyntheticReplyMessage()`).

Stand jetzt (geprüft am Datenfile): **141 Zeilen, 650 KB, 325 Tool-Aufrufe
insgesamt** (Ø 2,3 pro Zeile). In dieser Zeit wurden Tools mehrfach umbenannt,
zusammengelegt und die System-Prompt-Formulierung mehrfach überarbeitet (zuletzt:
Adjudication-Merge in `configure_tournament`/`configure_sprt`, Start/Stop- und
Status/Clear/Show-Konsolidierung auf `start`/`stop`/`get_status`/`clear_result`/
`show_result` mit `type`-Parameter). Die Datei spiegelt diese Historie 1:1 wider —
**jede** Zeile trägt den zum Aufnahmezeitpunkt aktiven System-Prompt-Wortlaut, und
gut die Hälfte der Zeilen ruft Tools auf, die es heute unter diesem Namen/dieser
Signatur nicht mehr gibt.

Gemessen (Stichprobe, exakte Zählung siehe unten): **77 von 141 Zeilen (55 %)**
enthalten mindestens einen veralteten Tool-Namen.

## Zwei getrennte Problemklassen

### A) Strukturell veraltete Tool-Aufrufe (mechanisch behebbar)

Tool wurde umbenannt oder in ein anderes Tool integriert; die Aktion selbst war
zum Aufnahmezeitpunkt korrekt, nur Name/Argument-Form stimmen nicht mehr mit dem
aktuellen Tool-Schema überein. Das ist rein mechanisch reparierbar, weil sich die
Zuordnung eindeutig aus dem alten Namen ableiten lässt.

### B) Inhaltlich fragwürdige Antworten ("schlechte Reaktionen")

Die KI-Antwort selbst ist unabhängig vom Tool-Schema fehlerhaft: Dialog-Mechanik
wird gegenüber dem Nutzer beschrieben (genau der in dieser Session behobene Bug),
Zahlen werden geraten statt aus einem Tool-Ergebnis übernommen, die Antwort ist
unnötig geschwätzig, oder — in mindestens einem gefundenen Fall — die Antwort ist
schlicht kaputt (Modell redet über den eigenen System-Prompt statt eine Aktion
auszuführen). Das lässt sich nicht mechanisch reparieren, nur heuristisch
vorsortieren; die Entscheidung "behalten / korrigieren / löschen" bleibt manuell.

Konkrete Funde in den echten Daten (Zeilennummern zum Zeitpunkt dieser Prüfung,
verschieben sich nach jeder Bearbeitung):

- Zeile 61: *"Die Dialoge zum Auswählen der neuen Eröffnungs- und PGN-Datei wurden
  geöffnet. Bitte wählen Sie die gewünschten Dateien aus."* — genau das
  Dialog-Narrations-Muster, das der neue `CRITICAL`-Absatz/die `terminal`-Logik
  verhindern soll.
- Zeile 108: Antworttext beginnt mit *"Sie haben absolut Recht, danke für den
  Hinweis. Im System-Prompt steht tatsächlich: ..."* — das Modell kommentiert
  seinen eigenen System-Prompt statt zu handeln. Eindeutiger Löschkandidat, nicht
  reparabel.
- Zeile 110, 115, 116: weitere Dialog-Narrationen ("Bitte wählen Sie nun die
  gewünschte Datei aus dem Datei-Auswahl-Dialog.", "Der Dialog wurde
  abgebrochen...").

## Mapping-Tabelle: alt → neu

Alle in den Daten gefundenen veralteten Tool-Namen mit Zielzustand. `type` wird bei
den zusammengelegten Tools aus dem alten Namen synthetisiert (steht nicht in den
alten Argumenten, muss injiziert werden).

| Alt | Alte Argumente (Beispiel) | Neu | Neue Argumente |
|---|---|---|---|
| `start_tournament` | `{}` | `start` | `{"type":"tournament"}` |
| `stop_tournament` | `{"mode":"abrupt"}` | `stop` | `{"type":"tournament","mode":"abrupt"}` |
| `start_sprt` | `{}` | `start` | `{"type":"sprt"}` |
| `stop_sprt` | `{"mode":...}` | `stop` | `{"type":"sprt","mode":...}` |
| `start_epd_analysis` | `{}` | `start` | `{"type":"epd"}` |
| `stop_epd_analysis` | `{"mode":...}` | `stop` | `{"type":"epd","mode":...}` |
| `get_tournament_status` | `{}` | `get_status` | `{"type":"tournament"}` |
| `get_sprt_status` | `{}` | `get_status` | `{"type":"sprt"}` |
| `get_epd_status` | `{}` | `get_status` | `{"type":"epd"}` |
| `clear_tournament_result` | `{}` | `clear_result` | `{"type":"tournament"}` |
| `clear_sprt_result` | `{}` | `clear_result` | `{"type":"sprt"}` |
| `clear_epd_result` | `{}` | `clear_result` | `{"type":"epd"}` |
| `show_tournament_result` | `{}` | `show_result` | `{"type":"tournament"}` |
| `show_sprt_result` | `{}` | `show_result` | `{"type":"sprt"}` |
| `show_epd_result` | `{}` | `show_result` | `{"type":"epd"}` |
| `configure_draw_adjudication` | `{"mode","min_full_moves","required_consecutive_moves","centipawn_threshold"}` | `configure_tournament` | dieselben Keys mit `draw_`-Präfix |
| `configure_resign_adjudication` | `{"mode","required_consecutive_moves","centipawn_threshold","two_sided"}` | `configure_tournament` | dieselben Keys mit `resign_`-Präfix |
| `configure_sprt_draw_adjudication` | wie oben | `configure_sprt` | `draw_`-Präfix |
| `configure_sprt_resign_adjudication` | wie oben | `configure_sprt` | `resign_`-Präfix |
| `open_tournament_openings_file_dialog` | `{}` | *(entfällt)* | als `configure_tournament`-Aufruf mit `{"openings_file_dialog":true}` modellieren |
| `open_tournament_pgn_file_dialog` | `{}` | *(entfällt)* | `configure_tournament` mit `{"pgn_file_dialog":true}` |
| `open_sprt_openings_file_dialog` | `{}` | *(entfällt)* | `configure_sprt` mit `{"openings_file_dialog":true}` |
| `open_sprt_pgn_file_dialog` | `{}` | *(entfällt)* | `configure_sprt` mit `{"pgn_file_dialog":true}` |
| `open_epd_file_dialog` | `{}` | *(entfällt)* | `configure_epd` mit `{"epd_file_dialog":true}` |

`open_add_engine_dialog` ist unverändert (nur `.terminal` ergänzt, kein
Namens-/Signaturwechsel) — keine Konvertierung nötig, nur die Tool-Antwort im
Datensatz ist stilistisch nicht mehr ganz aktuell (siehe unten, Kategorie B).

**Wichtig, auch wenn nicht als eigener Tool-Aufruf sichtbar:** Zeilen, die
`configure_tournament`/`configure_sprt` aufrufen, dessen `tool`-Antwort aber noch
im alten `"Configured: X, Y."`-Kurzformat vorliegt statt im heutigen vollständigen
Status-Text — das lässt sich nicht rückwirkend regenerieren (der reale App-Zustand
von damals ist weg). Diese Antworttexte bleiben als historischer Fakt stehen;
sie sind nicht falsch, nur nicht mehr repräsentativ für das aktuelle
Tool-Antwortformat. Einziges Gegenmittel: diese Zeilen beim Review als
"grenzwertig" markieren, nicht automatisch reparieren.

## Systemprompt

Jede Zeile trägt den zum Aufnahmezeitpunkt aktiven System-Prompt-Text als erste
Message (`role:"system"`) — von der sehr langen Ur-Fassung (siehe Datei-Anfang, ca.
2000+ Zeichen, "Respond in the user's configured language (code: eng)...") bis zur
heutigen kompakten Fassung. Für ein Fine-Tuning-Ziel, das mit dem *aktuellen*
System-Prompt ausgeliefert wird, sind alte Formulierungen ein Trainings/Inferenz-
Mismatch — das Modell würde auf einen Prompt-Wortlaut trainiert, den es zur
Laufzeit nie sieht. Empfehlung: **jede Zeile bekommt beim Aufräumen den aktuellen
System-Prompt-Text eingesetzt** (Quelle: `ensureController()` in
`src/chatbot/chatbot-llm-chat.cpp`, exakt der `std::format(...)`-String, mit dem
`languageCode` der jeweiligen Zeile befüllt). Das ist ein reiner Textersatz, ohne
Informationsverlust — der System-Prompt trägt keine Turn-spezifischen Daten.

## Empfehlung: Python-Tooling statt Punkt-für-Punkt

**Für Kategorie A (strukturell): Skript, nicht Handarbeit.** Begründung:

- 650 KB in 141 Zeilen à ein JSON-Objekt — von Hand in einem Editor bearbeiten
  heißt: einzelne, für Menschen kaum lesbare Zeilen voller escapter Anführungszeichen
  durchsuchen. Fehleranfällig, nicht verifizierbar außer durch erneutes Parsen.
- 325 Tool-Aufrufe, davon laut Zählung über 90 mit einem der 23 alten Namen — die
  Mapping-Regeln sind mechanisch, aber die schiere Zahl macht Handarbeit
  unpraktikabel und nicht wiederholbar (falls später noch mehr alte Daten
  auftauchen, oder falls sich beim nächsten Tool-Umbau dasselbe Problem
  wiederholt — was bei diesem Projekt inzwischen ein wiederkehrendes Muster ist).
- Ein Skript ist verifizierbar: nach dem Lauf kann man exakt zählen "0 Treffer für
  alte Namen mehr", was man von Hand nicht verlässlich behaupten kann.

**Für Kategorie B (inhaltlich): Heuristik-gestützte Vorauswahl, manuelle
Entscheidung.** Ein Skript kann verdächtige Zeilen finden (siehe Heuristiken
unten), aber ob eine Antwort wirklich schlecht ist, geschwätzig, oder nur
knapp-aber-okay, ist eine Qualitätsentscheidung, die ich (oder der Nutzer) pro
Fund treffen sollte — bei 141 Zeilen ist das ein machbarer manueller Umfang, sobald
die Kandidatenliste klein ist (die Heuristiken dürften auf einen Bruchteil der 141
Zeilen eingrenzen, siehe die 4-5 echten Funde oben als grobe Größenordnung).

## Geplante Skripte (Ablage-Vorschlag: `scripts/finetuning/`, reines Wegwerf-Tooling,
nicht Teil des CMake-Builds)

1. **`migrate_tool_calls.py`**
   - Liest `finetuning.json` zeilenweise, parst jede Zeile als JSON.
   - Für jede `assistant`-Message: jeden `tool_calls[]`-Eintrag gegen die
     Mapping-Tabelle prüfen; bei Treffer `function.name` ersetzen und
     `function.arguments` (JSON-String) um die passenden Präfixe/`type`-Injektion
     transformieren.
   - Ersetzt zusätzlich den `system`-Message-Inhalt jeder Zeile durch den aktuellen
     Prompt-Text (Sprachcode aus der alten Message extrahiert/beibehalten, falls
     unterscheidbar, sonst Default).
   - Reine Text-/Strukturersetzung in `content`-Feldern (z. B. wenn ein alter
     Tool-Name in einer `tool`-Antwort oder `assistant`-Antwort als Klartext
     vorkommt, z. B. *"...ruft dann open_tournament_openings_file_dialog auf"*)
     nach derselben Mapping-Tabelle, als zusätzlicher Sicherheitsnetz-Pass.
   - Schreibt Ausgabe in eine **neue** Datei (`finetuning.migrated.json`), rührt
     das Original nicht an.
   - Gibt am Ende eine Zusammenfassung aus: wie viele Zeilen verändert, wie viele
     Tool-Aufrufe pro altem Namen ersetzt, wie viele Zeilen weiterhin (warum auch
     immer) einen unbekannten/nicht gemappten Namen enthalten (sollte 0 sein —
     alles andere ist ein Signal, die Mapping-Tabelle zu ergänzen, bevor
     weitergemacht wird).

2. **`flag_review_candidates.py`**
   - Liest die migrierte Datei, durchsucht jede `assistant`-`reply_to_user`-Antwort
     nach Heuristiken:
     - Wörter wie "Dialog", "dialog", "Datei-Auswahl" in einer Antwort, obwohl der
       vorangegangene Tool-Aufruf kein `open_add_engine_dialog`/`open_pgn_file`
       war (also die "keine Dialog-Erwähnung"-Regel betrifft) → Dialog-Narration.
     - Zahlen (Regex `\d`) in der Antwort, die in keinem vorangegangenen
       `tool`-Message-Content vorkommen → möglicher Hallucination-Kandidat
       (Score/Elo/Ergebnis geraten statt übernommen).
     - Ungewöhnlich lange `reply_to_user`-Antworten (z. B. > 400 Zeichen) →
       Geschwätzigkeits-Kandidat.
     - Antworttext, der Formulierungen wie "im System-Prompt steht" oder "als
       KI-Assistent" enthält → Meta-Kommentar/kaputte Antwort wie der Zeile-108-Fund.
   - Gibt eine Liste `(Zeilennummer, Grund, Ausschnitt)` aus — als Markdown-Tabelle
     oder CSV, zur manuellen Durchsicht.

3. **`apply_review_decisions.py`** (erst nach der manuellen Durchsicht)
   - Nimmt eine kleine, von Hand gepflegte Entscheidungsliste (Zeilennummer →
     `drop` / `fix:<neuer Text>`) und wendet sie auf die migrierte Datei an, um die
     finale, bereinigte Datei zu erzeugen.

Alle drei Skripte sind bewusst klein und linear (kein Framework) — bei 141 Zeilen
lohnt sich keine komplexere Architektur.

## Sicherheitsnetz

- Original `finetuning.json` wird **nie in-place verändert**, alle Skripte
  schreiben neue Dateien. Erst nach manueller Prüfung (Diff der Zeilenzahl, ein
  paar Stichproben-Zeilen mit `python -m json.tool`) wird die migrierte Datei die
  echte `finetuning.json` ersetzen — und zwar durch den Nutzer, nicht automatisch
  durch mich.
- Vor dem finalen Ersetzen: Kopie des Originals mit Zeitstempel behalten (z. B.
  `finetuning.json.bak-2026-08-03`), falls sich später ein Mapping-Fehler zeigt.
- `migrate_tool_calls.py` bricht mit Fehlermeldung ab (statt still falsch zu
  mappen), wenn ein Tool-Name in `tool_calls` auftaucht, der weder in der aktuellen
  Tool-Liste noch in der Mapping-Tabelle steht — Schema-Drift soll aktiv auffallen,
  nicht durchrutschen.

## Offene Entscheidungen (bitte vom Nutzer festlegen)

1. **Zeilen wie Nr. 108 (kaputte Antwort):** löschen oder Turn komplett verwerfen?
   Empfehlung: löschen, nicht reparieren — ein rekonstruierter "richtiger" Text
   wäre keine echte Trainingsprobe mehr.
2. **Dialog-Narrations-Zeilen (61, 110, 115, 116, ...):** Antworttext durch eine
   korrekte, knappe Version ersetzen (`fix:`) oder ganz löschen (`drop`)? Da der
   zugrundeliegende Tool-Aufruf/Ergebnis meist noch verwendbar ist, tendiere ich zu
   `fix` (Text ersetzen durch eine knappe, faktenbasierte Bestätigung ohne
   Dialog-Erwähnung) statt kompletten Turn wegzuwerfen — muss aber jeweils den
   passenden Tool-Ergebnistext danebenhalten, um keine erfundene Aussage
   einzusetzen.
3. **Alte `configure_tournament`/`configure_sprt`-Antworten im
   Kurzformat statt Vollstatus:** so lassen (historisch korrekt, nur nicht mehr
   repräsentativ) oder ebenfalls markieren/entfernen? Empfehlung: so lassen, außer
   die Review-Heuristik zeigt, dass sie negativ auffallen.
4. Soll ich das jetzt umsetzen (Skripte schreiben + laufen lassen +
   Review-Kandidaten vorlegen), oder nur der Plan für jetzt, Umsetzung als
   nächster, separat bestätigter Schritt?

## Ablauf-Checkliste

- [x] `migrate_tool_calls.py` schreiben und gegen `finetuning.json` laufen lassen
      → `finetuning.migrated.json`
- [x] Verifizieren: keine alten Tool-Namen mehr enthalten (`grep` der
      Mapping-Tabelle liefert 0 Treffer)
- [x] Stichprobe: migrierte Zeilen mit `python -m json.tool` validieren und
      inhaltlich gegenlesen
- [x] `flag_review_candidates.py` laufen lassen → Kandidatenliste
- [x] Kandidatenliste manuell durchgehen, Entscheidungen (`drop`/`fix`) notieren
- [x] `apply_review_decisions.py` anwenden → finale Datei
- [x] Backup des Originals anlegen, finale Datei an Original-Pfad übernehmen

## Ergebnis (umgesetzt am 2026-08-03)

- **Zusätzlicher Fund beim Umsetzen, der im Plan noch fehlte:** die alte
  `"openings_file":"dialog"`/`"pgn_file":"dialog"`-Sentinel-*Werte* innerhalb
  sonst unveränderter `configure_tournament`-Aufrufe wurden von der
  Namens-Mapping-Tabelle nicht erfasst (die matcht nur Tool-*Namen*, nicht
  Argument-Werte) — 3 Zeilen betroffen, extra behandelt (siehe unten).
- **Migration (`migrate_tool_calls.py`):** alle 141 Zeilen migriert. System-Prompt
  jeder Zeile durch den aktuellen Wortlaut ersetzt; 109 Tool-Aufrufe über 23
  veraltete Namen (inkl. der in der Tabelle oben noch fehlenden
  `start_sprt_tournament`/`stop_sprt_tournament`, zusammen 11 Treffer) auf die
  aktuellen Namen/Argumentformen umgestellt. 0 unbekannte Tool-Namen danach.
- **Qualitäts-Review (`flag_review_candidates.py` + manuelle Entscheidung):**
  49 automatisch markierte Kandidaten, davon die meisten harmlose
  Falsch-Positive der "unsourced numbers"-Heuristik (Engine-Versionsnummern wie
  "Qapla 0.4.0" wurden als vermeintlich erfundene Zahlen erkannt — irrelevant,
  siehe "Werte sind nicht wichtig"). Echte Befunde: 8 Zeilen (Zeile 107 davon
  nur über den separaten Dialog-Sentinel-Scan gefunden, nicht über die
  Text-Heuristik).
  - **2 Zeilen gelöscht** (108, 119): reine Meta-Gespräche darüber, wie der
    Nutzer den System-Prompt/die KI umformulieren müsste, damit sie sich
    "richtig" verhält — kein Tool-Einsatz-Beispiel, gehört inhaltlich nicht in
    den Datensatz; eine erfundene "passende" Antwort hätte hier nur ein
    Fake-Meta-Gespräch simuliert statt ein echtes Trainingsbeispiel zu liefern.
  - **3 Zeilen strukturell repariert** (107, 110, 115): alte `"pgn_file":
    "dialog"`/`"openings_file":"dialog"`-Argumente auf die heutigen Boolean-Flags
    umgestellt, UND die anhängende `reply_to_user`-Nachricht entfernt — nach der
    heutigen `.terminal`-Logik bekommt das Modell nach so einem Aufruf gar keine
    weitere Anfrage mehr, die alte Anschlussantwort konnte also gar nicht mehr
    vorkommen.
  - **3 Zeilen mit korrigiertem Antworttext** (116, 117, 123): Dialog-Narration
    bzw. langatmige Meta-Entschuldigungen durch eine kurze, rein faktenbasierte
    Bestätigung ersetzt (Werte 1:1 aus dem jeweiligen Tool-Ergebnis derselben
    Zeile übernommen, nichts erfunden).
- **Ergebnis:** `finetuning.json` (139 Zeilen, vorher 141) enthält keine
  veralteten Toolnamen, keine alten Dialog-Sentinel-Werte, keine
  Dialog-Narration und keine Meta-Kommentare mehr — verifiziert durch erneuten
  Lauf von `flag_review_candidates.py` (0 Treffer) und vollständige
  JSON-Validierung aller Zeilen.
- **Backup:** Original vollständig gesichert unter
  `~/.qapla-chess-gui/finetuning.json.bak-2026-08-03` (141 Zeilen, unverändert).
- **Tooling:** Die drei Skripte liegen aktuell nur im Session-Scratchpad
  (Wegwerf-Tooling wie geplant), nicht im Repo. Bei Bedarf (z. B. falls sich das
  Muster beim nächsten Tool-Umbau wiederholt) sage Bescheid, dann lege ich sie
  unter `scripts/finetuning/` ab.
