# Qapla-spezifische Finetuning-Skripte

Diese drei kennen den Chatbot: seine Werkzeuge, seinen Systemprompt, den Aufbau der
Trainingsdaten. Sie sind auf ein anderes Vorhaben **nicht** übertragbar — dafür ist
`scripts/finetuning/` da.

| Skript | Aufgabe | Hängt ab von |
|---|---|---|
| `build_dataset.py` | `records.json` + `worlds.json` → Datensatz + Protokoll | Datenaufbau |
| `verify_dataset.py` | prüft den Datensatz gegen den Regelkatalog | Werkzeuge, Systemprompt |
| `check_tools.py` | `tools.json` gegen die C++-Quellen | `src/llm/` |

## Ablauf

```sh
python3 scripts/qapla-chatbot/check_tools.py     # stimmt tools.json noch?
python3 scripts/qapla-chatbot/build_dataset.py   # -> data/qapla-chatbot/generated/
python3 scripts/qapla-chatbot/verify_dataset.py  # erwartet 0 Beanstandungen
```

Danach übernimmt `scripts/finetuning/` (siehe dortiges README).

## Wozu `check_tools.py`

`tools.json` ist ein Auszug aus dem Chat-Log, also tatsächlich das, was über die Leitung
ging. Das ist zugleich seine Schwäche: Ein Auszug altert, sobald sich eine Beschreibung im
C++ ändert — unsichtbar. Genau das ist einmal passiert, und ausgerechnet bei der Regel, die
14 Records im Datensatz vorführen.

Der Vergleich ist bewusst grob: alle String-Literale aus `src/llm` einsammeln und prüfen, ob
jede Beschreibung darin vorkommt. So bricht ein Refactoring den Test nicht — die
Werkzeugdefinitionen wurden bereits einmal auf Unterverzeichnisse verteilt.

## Wozu `verify_dataset.py`

Der Regelkatalog ist aus Systemprompt und Werkzeugbeschreibungen abgeleitet. Die zentrale
Prüfung ist die Belegbarkeit: Jede Zahl und jeder Pfad in einer Antwort muss in einem
Werkzeugergebnis, der Nutzerfrage oder dem Kontext desselben Records vorkommen. Das ist
mechanisch entscheidbar und fängt genau die Halluzinationen, gegen die der Datensatz
geschrieben wurde.

Der Gegentest gehört dazu:

```sh
python3 scripts/qapla-chatbot/verify_dataset.py                                  # 0
python3 scripts/qapla-chatbot/verify_dataset.py --dataset data/qapla-chatbot/original.json   # 149
```

Ein Prüfer, der „0 Beanstandungen" meldet, ohne je an fehlerhaften Daten gezeigt zu haben,
dass er anschlägt, beweist nichts.
