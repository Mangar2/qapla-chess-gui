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

### 9. Ergebnis des ersten brauchbaren Adapters (37 Iterationen)

Der Adapter aus dem abgestürzten Lauf, Stand Iteration 37, fusioniert und in LM Studio
gemessen — gegen dieselben 36 Testfälle wie die Baseline:

| | Baseline | 37 Iterationen |
|---|---|---|
| **acted** (überhaupt ein Tool) | 36,1 % | **94,4 %** |
| **correct** (das richtige Tool) | 8,3 % | 5,6 % |

**Das Kernproblem ist weitgehend gelöst.** Von 23 Prosa-Antworten bleiben 2. Nach weniger als
einem Zehntel einer Epoche ruft das Modell praktisch immer ein Tool auf, statt zu behaupten.

**Die Tool-Auswahl ist es nicht.** 2 von 36 gegen 3 von 36 ist bei 12 unabhängigen Messpunkten
Rauschen, keine Verschlechterung. Das Fehlerbild ist systematisch:

```
get_status  ->  configure_tournament   9x
get_status  ->  configure_sprt         6x
start       ->  configure_tournament   4x
```

Das Modell hat die *Form* gelernt und rät bei der *Auswahl* das häufigste Tool
(`configure_tournament` 90 Aufrufe, `configure_sprt` 75). Klassisches Unterlernen.

Ob dahinter zusätzlich ein Designproblem steckt, ist offen: Das Tool-Set folgt zwei
Schnittachsen gleichzeitig — `configure_tournament`/`configure_sprt`/`configure_epd` tragen die
Domäne im *Namen*, `start`/`stop`/`get_status`/`clear_result`/`show_result` als *Parameter*.
Das Modell routet nach Domäne und landet beim Tool mit dem passenden Namen. Die Frage lässt
sich erst nach einem längeren Lauf entscheiden; ein Umbau würde 180 der ~400 Tool-Aufrufe im
Datensatz entwerten.

### 10. Warum es beim 37er-Stand bleibt: die Maschine trägt es nicht

Vier Anläufe für eine volle Epoche, keiner erfolgreich:

| Anlauf | Konfiguration | Ergebnis |
|---|---|---|
| 1 | 16 Layer, kein Limit | Kernel-Panic bei Iter ~10 |
| 2 | 8 Layer, Limit 10,66 GB | Kernel-Panic bei Iter ~50 |
| 3 | ohne Limit | Stillstand, keine 10 Iterationen in 100 min |
| 4 | Limit zurück, Gather zurückgerollt | Stillstand, keine 10 Iterationen in 45 min |

Der Befund aus Anlauf 3 und 4:

```
Swap:  ~4 GB von 5 GB dauerhaft belegt,  19.570 Pageouts
GPU:   100 % Auslastung
CPU:   10 Sekunden in 70 Minuten Laufzeit
```

Die GPU steht auf 100 %, **weil sie auf ausgelagerten Speicher wartet**. Das ist kein Rechnen.
Metal-Speicher taucht übrigens nicht im RSS auf — der Python-Prozess wirkt mit 0,63 GB
harmlos, während er ~14 GB Working Set hält. Deshalb bringt es auch fast nichts, andere
Anwendungen zu schließen: alle VSCode-Prozesse zusammen sind ~0,5 GB.

Messbar degradiert das System über die Läufe hinweg — dieselbe Validierung brauchte
**57 s → 113 s → 126 s** bei konstanter Swap-Belegung.

**Zwei Irrtümer, die Zeit gekostet haben**, hier festgehalten, damit sie nicht wiederholt
werden:

1. *„Die Logit-Matrix ist der Speicherfresser."* 7800 x 65536 sind rechnerisch 1,9 GB plus
   Gradient, gebraucht werden ~85 Zeilen. Die Umstellung auf gathered Logits war korrekt
   (numerisch bit-identisch verifiziert) und **wirkungslos**: 14,65 → 14,75 GB. MLX wertet
   verzögert aus und fusioniert; die Matrix wurde nie am Stück gehalten. Wieder entfernt.
2. *„Das Speicherlimit verursacht den Stillstand."* Es zu entfernen machte es **schlechter**
   (Anlauf 3). Das Limit zwingt MLX zu sparsamerem Verhalten; der Stillstand kam vom Swapping.

**Was tatsächlich hilft, ist ungetestet:** ein Neustart des Rechners. Der Swap steht seit
Stunden auf ~4 GB und wird nicht freigegeben, was jeden neuen Lauf vorbelastet.

### 11. Der Ausweg: PyTorch auf der CPU

Nachdem vier Anläufe auf der GPU gescheitert waren, der Wechsel — und er ist besser als ein
Kompromiss. Gemessen auf demselben M4 (10 Kerne), dieselben Daten, dieselbe Maskierung:

| Weg | pro Iteration | Speicher | Ausgang |
|---|---|---|---|
| MLX, GPU, 4-bit | 25–48 s | 14,7 GB | 2 Kernel-Panics, 2× Stillstand |
| MLX, CPU | — | — | **einkernig** (99 % von einem von zehn), keine Thread-Steuerung |
| PyTorch, CPU, float32 | **59–61 s** | **10,7 GB** | läuft |
| PyTorch, CPU, bfloat16 | **> 2 h** | 2,6 GB | unbrauchbar |

**Gleich schnell wie die GPU bei deutlich weniger Speicher.** Der Grund ist
`attn_implementation="sdpa"`: Die Attention-Matrix ist bei 7800 Token **3,63 GB pro Schicht**
über 6 Attention-Schichten, und SDPA materialisiert sie nie. MLX hält sie offenbar — das war
der eigentliche Treiber des Peaks, nicht die Logits (siehe Abschnitt 10, Irrtum 1).

**bfloat16 ist auf ARM-CPU keine Option.** Der Speicher halbiert sich zwar (2,6 GB), aber
PyTorch hat dafür keine optimierten Kernel und emuliert: in knapp 6 Stunden keine 3
Iterationen, gegen 60 Sekunden unter float32. Das ist ein Faktor von über 100.

`train_torch.py` ist dabei **kürzer** als `train_lora.py`, nicht länger: Die Maskierung ist
`labels = -100`, was `CrossEntropyLoss` nativ ignoriert. Der ganze Eigenbau — Custom-Loss,
Custom-Batching, Index-Vorberechnung — entfällt, und mit ihm die zwei Fehler, die er
produziert hatte (Array-Auswertung unter `value_and_grad`, zufällige Validierungsreihenfolge).
Gegengeprüft: identische Maskierung, 31.629 von 2.801.333 Token in beiden Implementierungen.

### 12. Hardware-Empfehlung

**Auf 16 GB bleibt es knapp**, auch auf der CPU: 10,7 GB Bedarf plus System heißt Auslagerung,
sobald sonst noch etwas läuft. Es funktioniert, aber ohne Reserve.

Für dieses Training gilt: **RAM schlägt Rechenleistung, und CPU schlägt eine zu kleine GPU.**

- **64 GB Windows / 32 GB Linux, CPU-only** — die richtige Wahl. 10,7 GB sind dort unkritisch,
  16 echte Kerne gegen 10 dürften die 60 s je Iteration unterbieten. Dasselbe Skript,
  `pip install torch peft transformers`, sonst nichts. Eine vorhandene GPU wird schlicht
  ignoriert.
- **8 GB AMD-GPU** — nicht brauchbar. 8 GB VRAM gegen ~11 GB Bedarf passt nicht, und
  Systemspeicher hilft einer dedizierten Karte nicht. Dazu ROCm unter Windows unreif.

Für LM Studio muss ein PyTorch-Ergebnis anschließend nach GGUF konvertiert werden
(llama.cpp); für LFM2 gibt es offizielle GGUF-Builds, der Weg ist also gangbar.

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

- Fusionieren und Deployment funktionieren, inklusive Template-Prüfung
- Gemessen: **acted 36,1 % → 94,4 %** beim 37-Iterationen-Stand

**Offen**

1. **Ein längerer Lauf.** Vier Anläufe gescheitert (Abschnitt 10). Nächster Schritt wäre ein
   Neustart des Rechners, danach ein Versuch aus sauberem Speicherzustand. Ohne das ist auf
   diesem Gerät kein längeres Training möglich; die Alternative wäre anderswo zu trainieren.
2. **Die Tool-Auswahl.** Erst ein längerer Lauf zeigt, ob die Domänen-Verwechslung
   Unterlernen ist oder das Tool-Design (Abschnitt 9).
3. **Lernrate.** Steht unverändert auf `1e-4` und wurde nie variiert.
4. **Kontrolllauf `--mask none`.** Wenn die Maskierung so entscheidend ist wie hier
   behauptet, muss ein unmaskierter Lauf deutlich schlechter abschneiden. Bisher ist das
   Argument plausibel, aber ungeprüft — und angesichts der Laufzeitprobleme teuer.
5. **Baseline unter gleichen Bedingungen.** Die Baseline lief mit unbekannter Kontextlänge,
   das getunte Modell mit 12288. Für den `acted`-Sprung irrelevant, sauber wäre es trotzdem.

## Wo was liegt

| Ort | Inhalt | Abhängig von | Git |
|---|---|---|---|
| `scripts/finetuning/` | Aufteilen, Trainieren, Fusionieren, Messen | nichts — alles über Parameter | versioniert |
| `scripts/qapla-chatbot/` | Datensatz bauen und prüfen, Werkzeug-Abgleich | Qapla: `src/llm`, Datenaufbau | versioniert |
| `data/qapla-chatbot/` | Records, Welten, Systemprompt, Werkzeuge, Ergebnisse | — nur Daten | versioniert |
| `Finetuning/` | venv, Splits, Adapter, fusionierte Modelle, Logs | — | **ignoriert** |

Die Trennung ist der Punkt: Ein Finetuning mit einem anderen Modell braucht nur andere
Parameter an `scripts/finetuning/`. Eine Umstrukturierung der Trainingsdaten betrifft
`data/qapla-chatbot/records.json` — eine Datei, kein Skript.

Bedienung: `scripts/finetuning/README.md` und `scripts/qapla-chatbot/README.md`.
