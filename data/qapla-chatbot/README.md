# Trainingsdaten für den Qapla-Chatbot

Nur Daten, kein Code. Die Skripte liegen in `scripts/qapla-chatbot/` (kennen diese Daten)
und `scripts/finetuning/` (kennen sie nicht).

## Dateien

| Datei | Inhalt | Von Hand bearbeiten? |
|---|---|---|
| `records.json` | **Die 151 Basis-Records.** Nutzerfrage, Werkzeugkette, Antwort, Begründung | **ja — hier wird gearbeitet** |
| `worlds.json` | Wertevariation und Umformulierungen | ja |
| `system-prompt.txt` | der Systemprompt, wortgleich in jedem Record | ja |
| `tools.json` | Werkzeugbeschreibungen, wie die App sie sendet | nein, aus dem Chat-Log |
| `chat_template.jinja` | Chat-Vorlage für LFM2.5 | nein |
| `original.json` | Rohdaten vor der Kuratierung | nein, Kontrollgruppe |
| `generated/` | erzeugter Datensatz und Review-Protokoll | nein, `build_dataset.py` |
| `results/` | Messergebnisse zum Vergleichen | nein |

## Aufbau eines Records

```json
{
  "index": 3,
  "user": "ja, concurrency auf 10 bitte",
  "context": [["bitte starte das turnier", "Die Concurrency steht auf 0 ..."]],
  "steps": [
    {"tool": "configure_tournament", "args": {"concurrency": 10}, "result": "..."},
    {"tool": "start", "args": {"type": "tournament"}, "result": "..."}
  ],
  "reply": "Concurrency steht auf 10, das Turnier läuft.",
  "note": "Kontext vorangestellt -- erst damit ist das \"ja\" verständlich."
}
```

- `context` — vorangehende Turns als geflachtete Texte, genau wie `LlmChatController` sie
  zur Laufzeit baut. Darf die alte, falsche Antwort enthalten: Kontext ist Eingabe, nie
  Trainingsziel.
- `reply: null` — der Turn endet an einem terminalen Werkzeug (Dateidialog). Das Modell
  wird danach nicht erneut gefragt, eine Antwort kann es also nicht geben.
- `note` — die Begründung der Korrektur, landet im Review-Protokoll, nicht im Datensatz.

## Grundregel der Kuratierung

> **Die Fehlersituation bleibt drin. Korrigiert wird nur die Zielantwort.**

Finetunen muss man, was noch nicht funktioniert. Ein Datensatz aus lauter gelungenen Turns
trainiert das, was ohnehin klappt. Kein Record wurde gelöscht.

Das gespeicherte Ziel ist aber immer das *korrekte* Verhalten — eine kaputte Antwort als
Ziel wäre die Anweisung, den Fehler zu reproduzieren.

## Wertevariation

Alle Records stammen aus denselben Sitzungen und trugen fast immer dieselben Werte. Das
arbeitet gegen den Zweck: Der Datensatz soll beibringen, nichts zu behaupten, was kein
Werkzeug gesagt hat — würde dem Modell diese konkreten Werte aber als Standard antrainieren.

`worlds.json` gibt deshalb jeden Record zusätzlich in zwei weiteren Welten aus, mit anderen
Engines, Pfaden und Zeitkontrollen. Die Ersetzung läuft über den **ganzen** Record, damit er
in sich stimmig bleibt. Reihenfolge der Regeln beachten: längere Muster zuerst, sonst frisst
`Qapla` das `Qapla-baseline`.

52 Records enthalten überhaupt keinen austauschbaren Wert („beende die anwendung"). Dort
lieferten die Welten dreimal dasselbe. Für sie steht in `paraphrases` je Welt eine eigene
Formulierung — der Werkzeugaufruf bleibt gleich, nur Frage und Antwort ändern sich. Genau
die Vielfalt, die parameterlosen Werkzeugen sonst fehlt.

## Ändern

```sh
# records.json bearbeiten, dann:
python3 scripts/qapla-chatbot/build_dataset.py
python3 scripts/qapla-chatbot/verify_dataset.py        # erwartet 0
```

Ändern sich Werkzeugnamen oder Argumente im C++, ist der Datensatz entwertet, bis
`records.json` nachgezogen ist. Das prüft `check_tools.py` für `tools.json`, nicht aber für
die Records — dort hilft nur `verify_dataset.py`, das unbekannte Werkzeuge und Argumente
meldet.
