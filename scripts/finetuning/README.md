# Finetuning-Werkzeuge

Generisch: **kennen weder ein bestimmtes Modell noch einen bestimmten Datensatz.** Alles
Konkrete kommt über Parameter, jedes Skript erklärt sich mit `--help`.

Was Qapla-spezifisch ist, liegt in `scripts/qapla-chatbot/`. Erfahrungen aus dem ersten
Durchlauf stehen in `docs/finetuning-training.md`.

| Skript | Aufgabe |
|---|---|
| `split.py` | Datensatz → train/valid/test, hält zusammengehörige Zeilen zusammen |
| `train.py` | LoRA-Training auf CPU |
| `fuse.py` | Adapter einrechnen, umwandeln, für LM Studio ablegen |
| `evaluate.py` | messen: wird gehandelt, und wird richtig gehandelt |

## Ablauf

```sh
V=Finetuning/.venv/bin/python

$V scripts/finetuning/split.py \
     --dataset data/qapla-chatbot/generated/dataset.jsonl \
     --tools   data/qapla-chatbot/tools.json \
     --out     Finetuning/data --parse-arguments

$V scripts/finetuning/train.py \
     --data Finetuning/data --adapter-path Finetuning/adapters \
     --template data/qapla-chatbot/chat_template.jinja --iters 200

$V scripts/finetuning/fuse.py \
     --adapter Finetuning/adapters --name mein-modell \
     --template data/qapla-chatbot/chat_template.jinja

$V scripts/finetuning/evaluate.py \
     --dataset Finetuning/data/test.jsonl --out ergebnis.json
```

Umgebung einmalig:

```sh
python3 -m venv Finetuning/.venv
Finetuning/.venv/bin/pip install torch peft transformers mlx-lm
```

`Finetuning/` ist reines Arbeitsverzeichnis und in `.gitignore` — alles darin lässt sich
neu erzeugen.

## Drei Dinge, die leicht schiefgehen

**Die Chat-Vorlage.** Manche Modelle liefern eine aus, die `tool_calls` verschluckt und
jeden Assistenz-Turn leer rendert. Das Training läuft dann durch, der Verlust fällt
unauffällig, und das Modell lernt zu schweigen. `--template` mit einer geprüften Vorlage
setzen; `fuse.py` prüft es und bricht ab, wenn Aufrufe verlorengehen.

**Der Split.** Enthält ein Datensatz jeden Fall in mehreren Varianten und landet eine im
Training, die andere in der Validierung, misst die Validierung Auswendiggelerntes.
`split.py` hält Zeilen mit gleichem `group`-Wert zusammen.

**Die Maskierung.** Bei langen Prompts sind die Assistenz-Antworten oft nur ein Prozent der
Token. Ohne `--mask assistant` geht fast das gesamte Lernsignal in das Nachsprechen des
Prompts.

## Speicherbedarf

```
M ≈ 7 GB × Parameterzahl[Mrd] + 2-3 GB
```

Gemessen: 1,2-Mrd-Modell bei 7.800 Token Kontext → 10,7 GB. Der zweite Summand trägt den
Kontext. Auf CPU nur `float32` verwenden — `bfloat16` ist dort um Größenordnungen langsamer.
