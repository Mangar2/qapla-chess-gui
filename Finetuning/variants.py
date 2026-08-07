"""Wertevariation gegen Auswendiglernen.

Alle Records stammen aus denselben Sitzungen und tragen deshalb fast immer dieselben
Werte: "Qapla 0.4.0" in 46 Records, book8ply.raw in 51, "H1=5.00" in jedem SPRT-Record.
Das arbeitet gegen den Zweck des Datensatzes -- er soll beibringen, nichts zu sagen, was
kein Tool gesagt hat, würde dem Modell diese konkreten Werte aber als Default antrainieren.
Genau die hat es in den Records 135 und 147 frei erfunden.

Deshalb wird jedes Record zusätzlich in mehreren "Welten" ausgegeben: andere Engines,
andere Pfade, andere Zeitkontrollen, andere Elo-Schranken. Die Ersetzung läuft über den
GESAMTEN Record -- User-Nachricht, Kontext, Tool-Argumente, Tool-Ergebnisse und Antwort --
damit die innere Konsistenz erhalten bleibt. Das ist die einzige Eigenschaft, auf die es
ankommt: die Antwort muss weiterhin ausschließlich aus dem stammen, was die Tools in
diesem Record gemeldet haben.

Ob das gelungen ist, entscheidet nicht dieses Modul, sondern verify.py: eine
inkonsistente Ersetzung hinterlässt eine Zahl oder einen Pfad in der Antwort, für die es
keine Deckung mehr gibt, und genau darauf prüft es.

Reihenfolge der Regeln ist wichtig -- längere Muster zuerst, sonst frisst "Qapla" das
"Qapla-baseline" auf.
"""

# (Suchmuster, Ersetzung) je Welt, in Anwendungsreihenfolge.
WORLDS = [
    ("stockfish-berserk", [
        # Engines -- lange Namen zuerst, danach die informellen Kurzformen aus den
        # User-Nachrichten ("qapla 0.4", "spike").
        ("Qapla-baseline", "Stockfish-dev"),
        ("Qapla 0.4.0", "Stockfish 16.1"),
        ("qapla 0.4.0", "stockfish 16.1"),
        ("qapla 0.4", "stockfish 16"),
        ("Spike 1.4.1", "Berserk 13"),
        ("qapla", "stockfish"),
        ("spike", "berserk"),
        # Pfade -- vollständige Pfade zuerst, dann der nackte Dateiname aus Record 44.
        ("/Users/volkerbohm/dev/qapla/engine-tester/test/opening/book8ply.raw",
         "/home/vb/chess/books/8moves_v3.pgn"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/src/test-system/test-data/wmtest.epd",
         "/home/vb/chess/epd/sts.epd"),
        ("/Users/volkerbohm/dev/qapla/engine-tester/test/epd/wmtest.epd",
         "/home/vb/chess/epd/bratko.epd"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/log/aitest2.pgn",
         "/home/vb/chess/games/run2.pgn"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/log/aitest3.pgn",
         "/home/vb/chess/games/run3.pgn"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/log/aitest.pgn",
         "/home/vb/chess/games/run1.pgn"),
        ("aitest3.pgn", "run3.pgn"),
        ("wmtest.epd", "sts.epd"),
        ("book8ply.raw", "8moves_v3.pgn"),
        # Zeitkontrollen -- die punktierten Schreibweisen zuerst.
        ("20.0+0.01", "300.0+3.0"),
        ("20+0.01", "300+3"),
        ("60.0+0.0", "180.0+2.0"),
        ("60+0", "180+2"),
        ("20s + 10ms increment", "5 minuten + 3 sekunden increment"),
        # Statistik. "10000" und "H1=5.00" kommen nur als max_games bzw. Elo-Schranke vor,
        # sind also eindeutig; "Max games: 4" nur als Phrase.
        ("H1=5.00", "H1=8.00"),
        ("Max games: 10000", "Max games: 40000"),
        ("Max. Spiele: 10000", "Max. Spiele: 40000"),
        ("10000", "40000"),
        ("Max games: 4.", "Max games: 6."),
        ("Max. Spiele: 4\n", "Max. Spiele: 6\n"),
        # Record 68/69 nennt den Wert im Fließtext und einmal ausgeschrieben.
        ("Max. Spiele steht auf 4.", "Max. Spiele steht auf 6."),
        ("nach vier Partien", "nach sechs Partien"),
        ("Event name: ai-test", "Event name: regression-run"),
        ("Event-Name: ai-test", "Event-Name: regression-run"),
        ("\"ai-test\"", "\"regression-run\""),
        ("auf ai-test", "auf regression-run"),
    ]),
    ("ethereal-rubi", [
        ("Qapla-baseline", "Ethereal-tune"),
        ("Qapla 0.4.0", "Ethereal 14"),
        ("qapla 0.4.0", "ethereal 14"),
        ("qapla 0.4", "ethereal 14"),
        ("Spike 1.4.1", "RubiChess 2.2"),
        ("qapla", "ethereal"),
        ("spike", "rubichess"),
        ("/Users/volkerbohm/dev/qapla/engine-tester/test/opening/book8ply.raw",
         "/data/chess/openings/uho_2022.epd"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/src/test-system/test-data/wmtest.epd",
         "/data/chess/epd/eret.epd"),
        ("/Users/volkerbohm/dev/qapla/engine-tester/test/epd/wmtest.epd",
         "/data/chess/epd/arasan.epd"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/log/aitest2.pgn",
         "/data/chess/pgn/match-b.pgn"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/log/aitest3.pgn",
         "/data/chess/pgn/match-c.pgn"),
        ("/Users/volkerbohm/dev/qapla/chess-gui/log/aitest.pgn",
         "/data/chess/pgn/match-a.pgn"),
        ("aitest3.pgn", "match-c.pgn"),
        ("wmtest.epd", "eret.epd"),
        ("book8ply.raw", "uho_2022.epd"),
        ("20.0+0.01", "40/120.0+1.0"),
        ("20+0.01", "40/120+1"),
        ("60.0+0.0", "10.0+0.1"),
        ("60+0", "10+0.1"),
        ("20s + 10ms increment", "40 züge in 2 minuten, danach 1 sekunde pro zug"),
        ("H1=5.00", "H1=2.50"),
        ("Max games: 10000", "Max games: 2500"),
        ("Max. Spiele: 10000", "Max. Spiele: 2500"),
        ("10000", "2500"),
        ("Max games: 4.", "Max games: 8."),
        ("Max. Spiele: 4\n", "Max. Spiele: 8\n"),
        ("Max. Spiele steht auf 4.", "Max. Spiele steht auf 8."),
        ("nach vier Partien", "nach acht Partien"),
        ("Event name: ai-test", "Event name: nightly"),
        ("Event-Name: ai-test", "Event-Name: nightly"),
        ("\"ai-test\"", "\"nightly\""),
        ("auf ai-test", "auf nightly"),
    ]),
]


def substitute(text, rules):
    for needle, replacement in rules:
        text = text.replace(needle, replacement)
    return text


def _map_value(value, rules):
    if isinstance(value, str):
        return substitute(value, rules)
    if isinstance(value, list):
        return [_map_value(item, rules) for item in value]
    if isinstance(value, dict):
        return {key: _map_value(item, rules) for key, item in value.items()}
    return value


def apply_world(record, name, rules):
    """Ein Record in einer anderen Welt -- jedes Textfeld durchläuft dieselben Regeln."""
    return {
        "index": record["index"],
        "variant": name,
        "user": substitute(record["user"], rules),
        "reply": None if record["reply"] is None else substitute(record["reply"], rules),
        "context": [(substitute(u, rules), substitute(a, rules))
                    for u, a in record["context"]],
        "steps": [(tool, _map_value(args, rules), substitute(result, rules))
                  for tool, args, result in record["steps"]],
        "note": record["note"],
    }
