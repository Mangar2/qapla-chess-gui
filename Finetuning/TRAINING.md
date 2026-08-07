# Finetuning des Chat-Modells — Ablauf, Erkenntnisse, Stand

Begleitdokument zu `README.md`: dort geht es um den **Datensatz**, hier um das **Training**
damit. Stand: 2026-08-07, Vorbereitung abgeschlossen, Trainingslauf offen.

## Worum es geht

Das Chat-Modell soll aufhören zu behaupten statt zu handeln. Statt dem Modell das über
immer neue Formulierungen im Systemprompt beizubringen, wird es an Beispielen trainiert —
den 453 kuratierten Records aus `finetuning-001.json`.

Trainiert wird per **LoRA** (Low-Rank Adaptation, nichts mit dem Funkstandard LoRa zu tun).
Die 1,2 Milliarden Gewichte des Basismodells bleiben eingefroren; gelernt werden nur ~11
Millionen Zusatzparameter, die neben die bestehenden Schichten gehängt werden. Statt eine
2048×2048-Matrix komplett zu ändern, lernt man ihre Änderung als Produkt zweier schmaler
Matrizen (2048×16 mal 16×2048) — daher „low rank", und daher passt das Ganze überhaupt auf
einen 16-GB-Rechner.

Was LoRA damit kann und was nicht: „ruf in dieser Situation das richtige Tool auf" ist ein
enges, konsistentes Verhaltensmuster und dafür gut geeignet. Ein neues Fachgebiet beibringen
könnte man so nicht.

### Die Kette

```
finetuning-001.json          453 Records, kuratiert (siehe README.md)
   + tools.json              die Tool-Schemas, die die App zur Laufzeit mitsendet
        |  prepare_mlx.py
        v
data/{train,valid,test}      381 / 36 / 36 Zeilen, Split über Basis-Records
        |  train_lora.py     LoRA-Training, Loss nur auf Assistant-Tokens
        v
adapters/                    ~11M gelernte Parameter (wenige MB)
        |  fuse_deploy.py    fusioniert in die Basisgewichte, installiert für LM Studio
        v
~/.lmstudio/models/qapla/    ladbares Modell
        |  eval_lmstudio.py  misst gegen dieselben Testfälle wie vorher
        v
Vorher/Nachher-Zahl
```

**Wichtig:** LM Studio selbst kann kein Finetuning — es ist reine Inferenz. Trainiert wird
mit `mlx-lm` auf der GPU des Macs; LM Studio kommt erst am Ende wieder ins Spiel, um das
fertige Modell zu servieren.

## Was gemacht wurde

### 1. Die Tool-Schemas — was im Datensatz fehlte

Der Datensatz enthält nur `{"messages": [...]}`. Zur Laufzeit sendet die App aber zusätzlich
ein `tools`-Array mit den JSON-Schemas aller 17 Tools (`buildToolsJson()` in
`lm-studio-client.cpp`), und das LFM2.5-Chat-Template hängt es an den Systemprompt an:
`"\nList of tools: [{...}, {...}]"`.

Ohne dieses Feld hätte das Training einen Prompt gelernt, den das Modell im Betrieb nie zu
sehen bekommt — derselbe Train/Inference-Mismatch, wegen dem im Cleanup schon der
Systemprompt in allen Records vereinheitlicht wurde, nur für den Teil, der aus dem C++-Code
kommt statt aus dem Datensatz. `mlx-lm` liest ein `tools`-Feld pro Zeile und reicht es an
`apply_chat_template` durch (`tuner/datasets.py:59`), also wird es dort angehängt.

`tools.json` ist ein Auszug aus dem Chat-Log, also tatsächlich das, was über die Leitung
ging. Genau das ist auch seine Schwäche: ein Auszug altert, sobald sich eine Beschreibung im
C++ ändert, und zwar unsichtbar. Beim ersten Abgleich war das schon passiert —
`open_add_engine_dialog` trug noch den Wortlaut von vor der `.terminal`-Umstellung, dem der
Satz *„ENDS YOUR TURN: … No further tool call, no reply_to_user afterwards"* fehlte.
Ausgerechnet die Regel, die 14 Records im Datensatz demonstrieren. Das Modell hätte sie aus
dem Verhalten raten müssen, während sie zur Laufzeit im Prompt steht.

Daraus wurde `check_tools.py`: zieht alle String-Literale aus `src/llm`, verkettet sie und
prüft, ob jede Beschreibung aus `tools.json` darin vorkommt — Tool-Beschreibungen *und*
Argument-Beschreibungen. Bewusst grob, damit Refactoring im C++ den Check nicht bricht.

> **Gegen die eigene Prüfung testen.** Eine erste Fassung meldete 16 Fehlalarme (Suche nach
> `"name"` mit Anführungszeichen, obwohl der Blob nur Literal-*Inhalte* enthält) und
> übersah später `reply_to_user`, weil dessen Schema in einem Raw-String `R"(...)"` steht,
> den der Regex für normale Literale zerreißt. Der Check läuft deshalb einmal gegen eine
> absichtlich verfälschte Kopie und muss dabei anschlagen — dieselbe Logik, die `verify.py`
> mit dem Gegentest gegen `original.json` schon anwendet.

Aktueller Stand: **17/17 identisch**, Systemprompt im Datensatz byte-identisch mit
`chatbot-llm-chat.cpp` (1588 Zeichen).

### 2. Aufteilung in Train / Valid / Test

Die 453 Zeilen sind 151 Basis-Records × 3 Welten, blockweise abgelegt (`build.py:135-137`):
Basis-Record *i* liegt auf den Zeilen *i*, *i+151*, *i+302*.

Ein zufälliger Split über Zeilen würde deshalb **garantiert lecken** — Welt A eines Records
im Training, Welt B in der Validierung. Der Validierungs-Loss würde dann auswendig Gelerntes
messen, gut aussehen und nichts bedeuten. `prepare_mlx.py` splittet daher über Basis-Records
und nimmt alle drei Welten mit.

Die Blocklage wird geprüft, nicht geglaubt: für alle 151 Records muss gelten, dass die drei
Welten dieselbe Tool-Kette in derselben Reihenfolge aufrufen, sonst bricht das Skript ab.
Stellt `build.py` je auf verschachtelte Ausgabe um, fällt das auf.

**Seltene Tools bleiben im Training.** `clear_result`, `close_application` und
`open_add_engine_dialog` haben je genau ein Basis-Record (die „Offenen Punkte" aus dem
README). Läge so eines im Holdout, fehlte dem Modell das Tool vollständig — und gemessen
wäre trotzdem nichts. Nicht gelöst: `select_engines` und `select_epd_engines` haben je vier
Records und liegen damit über der Pinning-Schwelle; einer davon kann im Holdout landen.

Ergebnis: **381 / 36 / 36 Zeilen** aus 127 / 12 / 12 Basis-Records.

### 3. Zwei Fallen im Chat-Template

Beide hätten das Training still ruiniert — mit unauffälligem Loss.

**Das Template im MLX-Repo verliert alle Tool-Aufrufe.** `LFM2.5-1.2B-Instruct-MLX-*` liefert
ein abgespecktes `chat_template.jinja` (1783 statt 5487 Bytes), das ausschließlich
`message["content"]` rendert und `tool_calls` gar nicht kennt. Ein Record sieht damit so aus:

```
<|im_start|>assistant
<|im_end|>                      ← configure_sprt: weg
<|im_start|>tool
SPRT test started...<|im_end|>
<|im_start|>assistant
<|im_end|>                      ← start, reply_to_user: weg
```

Trainiert man darauf, lernt das Modell exakt das Gegenteil des Ziels: schweigen und darauf
hoffen, dass Tool-Ergebnisse von selbst erscheinen. Das Template des Haupt-Repos
(`LiquidAI/LFM2.5-1.2B-Instruct`) kann es und liegt jetzt als `chat_template.jinja` hier im
Verzeichnis. **Es muss beim Deployen mit ins Modellverzeichnis**, sonst holt sich LM Studio
wieder das kaputte — `fuse_deploy.py` erzwingt das und verifiziert es.

**`arguments` muss ein Objekt sein, kein String.** Der Datensatz hält sich an die
OpenAI-Konvention (`"arguments": "{\"concurrency\":10}"`); das Template lehnt das explizit ab
(*„Tool call arguments must be a mapping, got a JSON-encoded string"*). `prepare_mlx.py`
parst sie beim Schreiben — nur fürs Rendern, der Datensatz selbst bleibt im Wire-Format.

Korrekt gerendert sieht ein Tool-Aufruf bei LFM2.5 **nicht wie JSON** aus, sondern
pythonisch:

```
<|tool_call_start|>[start(type='sprt')]<|tool_call_end|>
```

Ob LM Studio das zurück in OpenAI-`tool_calls` übersetzt, war die offene Risikofrage des
ganzen Ansatzes — **es tut es** (siehe Baseline: 13 von 36 Antworten kamen als echte
`tool_calls` zurück).

### 4. Nur 1,1 % der Tokens tragen Lernsignal

Ein Record hat ~7,4k Tokens. Davon sind ~7,1k Systemprompt plus Tool-Schemas — in allen 453
Records byte-identisch. **Assistant-Ausgabe, also das, was der Datensatz beibringen soll:
31.629 von 2.801.333 Tokens = 1,1 %.**

Ohne Maskierung bestünde das Trainingssignal zu 99 % daraus, die Tool-Schemas auswendig zu
lernen — die das Modell nie selbst erzeugen muss.

`mlx-lm` kann maskieren, aber nur **einen Präfix pro Sample** (`default_loss`:
`steps >= lengths[:, 0]`). Das kann „die vier getrennten Assistant-Abschnitte in diesem
Record" nicht ausdrücken: eine mehrstufige Tool-Kette ist Assistant → Tool → Assistant → …
Auch `--mask-prompt` hilft nicht, es maskiert alles bis zur letzten Nachricht und damit
genau die Zwischen-Tool-Calls, die gelernt werden sollen.

Deshalb bringt `train_lora.py` eigene `loss`- und `iterate_batches`-Funktionen mit (beides
sind Parameter von `train()`, mlx-lm muss nicht gepatcht werden). Die Maske läuft pro Token:

```python
mask = masks[:, 1:]          # verschiebt sich mit den Targets
ce = nn.losses.cross_entropy(logits, targets) * mask
return ce.sum() / mask.sum()
```

Die Spans kommen aus den `{% generation %}`-Markern des Templates. Verliert das Template sie
je, wären alle Masken leer und jeder Record trüge null Loss bei — das Skript bricht dann ab,
statt still auf nichts zu trainieren. `--mask prefix|none` gibt es als Kontrollgruppe.

### 5. Baseline vor dem Training

Ohne Vorher-Zahl ist „besser geworden" ein Gefühl. `eval_lmstudio.py` schickt für jeden
Testfall genau das, was die App am Turn-Anfang schickt, und vergleicht mit dem Datensatz:

```
model: liquid/lfm2.5-1.2b   (ungetunt)
acted (any tool call): 13/36 = 36.1%
correct first tool:     3/36 =  8.3%
```

- **acted** — kam überhaupt ein Tool-Aufruf statt Prosa. Das ist die eigentliche Zielgröße:
  die App kann dem Nutzer nur einen `reply_to_user`-Aufruf anzeigen, Prosa ist keine
  schlechtere Antwort, sondern keine.
- **correct** — und es war das Tool, das der Datensatz zuerst aufruft.

**23 von 36 Antworten waren Prosa**, dreimal davon ein `reply_to_user`, das schlicht als
Klartext kam — trotz `tool_choice: "required"`. Wo gehandelt wurde, oft daneben: neunmal
`get_status` erwartet, stattdessen Prosa; dreimal `configure_sprt` statt `get_status`, also
konfigurieren statt erst nachsehen — was der Systemprompt mit *„Don't ask what tools can tell
you"* ausdrücklich verbietet.

> **Zur Aussagekraft:** 36 Zeilen sind 12 Basis-Records × 3 Welten, und die Welten antworten
> fast durchweg identisch. Das sind **12 unabhängige Messpunkte, nicht 36**. Die 8,3 % sind
> ein einziger funktionierender Record (`open_pgn_file`). Für Vorher/Nachher taugt das, aber
> aus 8 % → 25 % darf man keine Nachkommastelle lesen.

Nur der erste Schritt wird bewertet. Die ganze Kette zu prüfen bräuchte die echten Tools, um
das jeweils nächste Ergebnis zu erzeugen — und nach einem Fehler bei Schritt 1 ist alles
Weitere ohnehin nicht mehr vergleichbar.

### 6. Hyperparameter: erst divergiert, dann gefallen

Der erste Lauf lief mit `scale: 20.0` — mlx-lms Default, gedacht für Rang 8, während hier
Rang 16 läuft. `scale` ist der Multiplikator auf den Adapter, `scale/rank` also die effektive
Schrittweite:

```
scale 20:  Train 1.614 → 3.991    Val 1.230 → 2.853
```

Nicht nur der Validierungs-Loss stieg, sondern auch der Trainings-Loss — das ist keine
Überanpassung, sondern Divergenz: das Modell wurde schlechter auf genau den Daten, die es
gerade sah. Mit `scale 2.0`:

```
scale 2:   Val 1.123 → 0.485 (Iter 20) → 0.706 (Iter 40)
```

Nach 20 Iterationen — nicht einmal ein Zwanzigstel einer Epoche — war der Val-Loss mehr als
halbiert. Das passt zum Charakter der Aufgabe: ein enges, konsistentes Verhaltensmuster sitzt
schnell. Der Wiederanstieg auf 0,706 bei Iteration 40 ist bei `batch_size 1` und acht
Validierungs-Batches noch Rauschen; **40 Iterationen erlauben keine Aussage über den
Verlauf**, nur darüber, dass die Konfiguration nicht mehr divergiert.

### 7. Speicher und Tempo — die Sequenzlänge dominiert

| | bf16 | 4bit |
|---|---|---|
| Gewichte | 2,4 GB | 0,62 GB |
| Peak-Speicher | 16,9 GB | 15,6 GB |
| pro Iteration | 38,5 s | 43,5 s |

Auf einem 16-GB-M4 heißt 16,9 GB Peak: das System swappt. Die naheliegende Antwort war 4-bit
— **sie hat nicht funktioniert.** Die Gewichte schrumpfen um 1,8 GB, der Peak aber nur um
1,3 GB, und schneller wurde es auch nicht.

Der Grund: Der Speicher wird nicht von den Gewichten dominiert, sondern von den Aktivierungen
über 7,8k Tokens Kontext — und die kommen zu ~96 % aus den Tool-Schemas in jedem Record.
Gegen die Sequenzlänge hilft keine Quantisierung. Trotzdem bleibt 4-bit die Wahl, weil es
etwas Luft schafft und nichts kostet.

**Daraus folgt die Laufzeit:** ~43 s pro Iteration, also **~4,6 Stunden pro Epoche** (381
Iterationen). Dazu kommen pro Lauf etwa 10 Minuten Fixkosten fürs Tokenisieren der 417
Records. Der 40-Iterationen-Testlauf brauchte 43,2 Minuten.

Eine Validierung kostet ~11 s pro Record. Über alle 36 Records sind das ~7 Minuten — die
erste Fassung validierte nach *jeder* Iteration und verbrachte damit mehr Zeit mit Messen als
mit Trainieren. Jetzt: höchstens 8 Batches, höchstens alle 25 Iterationen.

> **Fehldiagnose am Rande, mit Lerneffekt:** Ein Lauf schien zu hängen — 1,8 % CPU, 0,3 GB
> RSS, Main-Thread in `_pthread_cond_wait`. Ich habe ihn abgeschossen. Er lief einwandfrei:
> MLX rechnet auf der **GPU**, dort war Vollast, und das `_pthread_cond_wait` war das Warten
> darauf. Bei MLX sagen `ps`-CPU-Werte nichts über den Fortschritt. Ebenso: `| tail -30` an
> einem laufenden Prozess unterdrückt jede Ausgabe bis zum Ende — genau deshalb war der
> Zustand nicht sichtbar.

### 8. Kernel-Panic: das Working Set der GPU ist die eigentliche Grenze

Der erste 150-Iterationen-Lauf hat den Rechner nach ~9 Minuten heruntergerissen:

```
panic(cpu 6): "completeMemory() prepare count underflow" @IOGPUMemory.cpp:550
Panicked task: 40986 pages, 25 threads: pid 61960: Python
```

Die PID ist exakt die des Trainingslaufs. Der Kernel ist also im Kontext unseres Prozesses
gepanikt, im GPU-Speichercode. 26 Minuten zuvor lief zusätzlich ein **JetsamEvent** (macOS'
OOM-Killer) — echter Speicherdruck ist damit unabhängig belegt.

Der Auslöser dahinter:

```
max_recommended_working_set_size:  11,84 GB   (mx.device_info())
gemessener Peak:                   15,9 GB    ← ~34 % darüber
```

**Was daraus nicht folgt.** `prepare count underflow` ist ein *Zählerunterlauf*, kein
Out-of-Memory — ein Refcount- oder Race-Bug im Treiber, kein sauber abgewiesenes
Speicherlimit. Und „panicked task = Python" heißt nicht „Python war die Ursache": bei einem
Bug im gemeinsamen GPU-Speichermanager kassiert der Client den Panic, der als Nächstes
`completeMemory()` aufruft. Zum Absturzzeitpunkt lief parallel ein Remotedesktop — auch ein
GPU-intensiver Client, der durch dieselben Allokationspfade geht. Die wahrscheinlichste
Erklärung ist die Kombination, nicht eine Einzelursache.

**Gegenmaßnahmen** (Risikominderung, kein gezielter Fix — gegen eine Race Condition hilft nur,
dem Treiber weniger Gelegenheit zu geben):

| Maßnahme | Wirkung |
|---|---|
| `mx.set_memory_limit(90 % der Empfehlung)` | Rückstau statt Allokation. **Kein hartes Dach** — einzelne nötige Allokationen gehen durch |
| `mx.set_cache_limit(512 MB)` | ungenutzte Puffer zählen zum Working Set und schieben zwischen Iterationen darüber |
| `--lora-layers 8` statt 16 | Peak 15,9 → **14,5 GB**, und Iterationen von 43 s auf **32 s** |
| keine andere GPU-Last parallel | der plausibelste Einzelfaktor |

Der Peak liegt mit 14,5 GB **weiterhin über der Empfehlung**. Wirklich darunter käme man nur
über einen kürzeren Kontext — und das sind die Tool-Schemas, die aus Konsistenzgründen bleiben
müssen. Das ist die harte Grenze dieses Aufbaus auf 16 GB.

## Stand

**Fertig und geprüft**

- `tools.json` deckungsgleich mit dem Code, Drift-Check vorhanden und gegengetestet
- Splits leckfrei erzeugt, alle 453 Zeilen rendern verlustfrei (kein Tool-Aufruf geht
  verloren), max. 7784 Tokens → `--max-seq-length 8192`
- Datensatz besteht `verify.py` weiterhin mit 0 Beanstandungen (149 bei den Rohdaten)
- Trainingsweg funktioniert: Maskierung greift (1,1 %), LoRA greift (11,11M Parameter),
  Speicher hält, Loss fällt
- Baseline gemessen: **acted 36,1 % / correct 8,3 %**
- LM Studio parst LFM2.5-Tool-Calls korrekt — die Risikofrage des Ansatzes ist beantwortet

**Offen**

1. **Der eigentliche Trainingslauf.** Bisher nur Testläufe über 20 und 40 Iterationen. Wie
   lang, ist noch zu entscheiden: eine volle Epoche kostet ~4,6 h; angesichts des schnellen
   Abfalls in den ersten 20 Iterationen könnte ein kurzer Lauf (~150 Iterationen, knapp 2 h)
   mit anschließender Messung der bessere erste Schritt sein.
2. **Lernrate.** Steht unverändert auf `1e-4` und wurde nie variiert — nach der
   `scale`-Korrektur der nächste Kandidat, falls die Kurve unruhig bleibt.
3. **Fusionieren und Deployment.** `fuse_deploy.py` ist geschrieben, aber noch nie gelaufen.
4. **Die Ergebniszahl.** `eval_lmstudio.py --out tuned.json` gegen dieselben 36 Testfälle,
   und zwar am fusionierten Modell in LM Studio — nicht am Adapter in mlx-lm, sonst misst man
   nicht, was der Nutzer bekommt.
5. **Ob der Loss überhaupt das Richtige misst.** Fallender Loss heißt „die Zieltokens werden
   wahrscheinlicher", nicht „das richtige Tool wird aufgerufen". Nur Punkt 4 beantwortet die
   eigentliche Frage.
6. **Kontrolllauf `--mask none`.** Wenn die Maskierung so entscheidend ist wie hier
   behauptet, muss ein unmaskierter Lauf deutlich schlechter abschneiden. Bisher ist das
   Argument plausibel, aber ungeprüft.

## Dateien

| Datei | Zweck |
|---|---|
| `tools.json` | Tool-Schemas, wie die App sie sendet. Aus dem Chat-Log, nicht von Hand |
| `check_tools.py` | prüft `tools.json` gegen `src/llm` — Exit 1, wenn der Auszug veraltet ist |
| `chat_template.jinja` | das Template, das `tool_calls` rendert. **Muss mit deployt werden** |
| `prepare_mlx.py` | Datensatz → `data/{train,valid,test}.jsonl` (Split, `tools`, `arguments`) |
| `train_lora.py` | LoRA-Training mit Assistant-Maskierung |
| `fuse_deploy.py` | fusioniert, installiert für LM Studio, verifiziert das Template |
| `eval_lmstudio.py` | misst `acted`/`correct` gegen einen Split |
| `baseline.json` | Messung des ungetunten Modells, Vergleichsgrundlage |
| `.venv/` | mlx-lm 0.31.3 + transformers. Nicht eingecheckt |

### Benutzung

```sh
python3 check_tools.py                       # erwartet: "tools.json matches the code"
python3 prepare_mlx.py                       # -> data/
python3 train_lora.py --iters 150            # LM Studio vorher entladen: lms unload --all
python3 fuse_deploy.py                       # -> ~/.lmstudio/models/qapla/lfm2.5-1.2b-qapla
python3 eval_lmstudio.py --out tuned.json    # gegen baseline.json halten
```
