"""Corrected records 60-69."""
from build import R
from fixed_000 import STARTED_TOURNAMENT
from fixed_030 import tournament_status_2e, ENGINE_LIST
from fixed_040 import BOOK, WMTEST, PGN2

STARTED_SPRT_BESIDE_T = "SPRT test started. Currently: a tournament is running; an SPRT test is starting."


def sprt_full(openings, pgn, concurrency, max_games, adjudication,
              running="No SPRT test is currently running."):
    """SPRT status with max_games and adjudication mode as parameters."""
    if adjudication == "active":
        adj = ("Draw adjudication: active (min full moves=80, required consecutive moves=20, "
               "centipawn threshold=20). Resign adjudication: active (required consecutive "
               "moves=5, centipawn threshold=500, two-sided=no).")
    else:
        adj = ("Draw adjudication: off (min full moves=80, required consecutive moves=20, "
               "centipawn threshold=20). Resign adjudication: off (required consecutive "
               "moves=5, centipawn threshold=500, two-sided=no).")
    return (
        "Champion (comparison engine): Qapla 0.4.0. Challenger (engine under test): Spike 1.4.1. "
        "Time control: 60+0. Elo bounds: H0=0.00, H1=5.00. Alpha=0.050, Beta=0.050. "
        f"Max games: {max_games}. Model: normalized. "
        f"Openings file: {openings}. PGN output file: {pgn}. Concurrency: {concurrency}. "
        f"{running} {adj}"
    )


def sprt_settings(openings, pgn, concurrency, max_games, adjudication, running_line):
    adj = "aktiv" if adjudication == "active" else "aus"
    return (
        "- Champion: Qapla 0.4.0\n"
        "- Challenger: Spike 1.4.1\n"
        "- Zeitkontrolle: 60+0\n"
        "- Elo-Grenzen: H0=0.00, H1=5.00\n"
        "- Alpha: 0.050, Beta: 0.050\n"
        f"- Max. Spiele: {max_games}\n"
        "- Modell: normalized\n"
        f"- Eröffnungsdatei: {openings}\n"
        f"- PGN-Ausgabedatei: {pgn}\n"
        f"- Concurrency: {concurrency}\n"
        f"- Remis-Adjudication: {adj}\n"
        f"- Aufgabe-Adjudication: {adj}\n"
        f"\n{running_line}"
    )


def t_settings_book(concurrency, running_line):
    return (
        "- Engines: Qapla 0.4.0, Spike 1.4.1\n"
        "- Zeitkontrolle: 20+0.01\n"
        "- Spiele pro Paarung: 4\n"
        "- Runden: 1\n"
        "- Event-Name: (nicht gesetzt)\n"
        f"- Eröffnungsdatei: {BOOK}\n"
        f"- PGN-Ausgabedatei: {PGN2}\n"
        f"- Concurrency: {concurrency}\n"
        "- Remis-Adjudication: aus\n"
        "- Aufgabe-Adjudication: aus\n"
        f"\n{running_line}"
    )


def tournament_book_2e(concurrency, games, rounds, running, adjudication="off"):
    adj_draw = f"Draw adjudication: {adjudication} (min full moves=80, required consecutive " \
               "moves=20, centipawn threshold=20)."
    adj_resign = f"Resign adjudication: {adjudication} (required consecutive moves=5, " \
                 "centipawn threshold=500, two-sided=no)."
    return (
        "Engines: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline. Time control: 20+0.01. "
        f"Games per pairing: {games}. Rounds: {rounds}. Event name: (not set). "
        f"Openings file: {BOOK}. PGN output file: {PGN2}. "
        f"Concurrency: {concurrency}. {running} {adj_draw} {adj_resign}"
    )


RECORDS = [
    R(60,
      user="wir brauchen eine neue eröffungsdatei und eine neue pgn datei",
      steps=[("configure_tournament", {"openings_file_dialog": True},
              f"Openings file set to: {BOOK}")],
      reply=None,
      note="Zwei Dialoge in einem Turn sind unmöglich -- der erste ist terminal. Die alte "
           "Antwort war zudem reine Dialog-Narration (\"Bitte wählen Sie die gewünschten "
           "Dateien aus\"), obwohl die Dateien laut Tool-Ergebnis längst gewählt waren."),

    R(61,
      user="gib mir die einstellungen des turnier",
      steps=[("get_status", {"type": "tournament"},
              tournament_book_2e(1, 4, 1, "No tournament is currently running.").replace(
                  "Qapla 0.4.0, Spike 1.4.1, Qapla-baseline", "Qapla 0.4.0, Spike 1.4.1"))],
      reply="Aktuelle Turniereinstellungen:\n"
            + t_settings_book(1, "Das Turnier läuft derzeit nicht."),
      note="Die Antwort erfand zwei Felder: \"Zeichentafel: nein\" und "
           "\"Aufhängeadjudikation: aus\". Beide existieren nicht -- ersetzt durch die "
           "tatsächlichen Adjudication-Felder aus dem Tool-Ergebnis."),

    R(62,
      user="lass mich die pgn datei neu setzen",
      steps=[("configure_tournament", {"pgn_file_dialog": True},
              f"PGN output file set to: {PGN2}")],
      reply=None,
      note="Dialog-Aufruf ist terminal -- die anschließende Antwort konnte real nicht mehr "
           "stattfinden und wurde entfernt."),

    R(63,
      user="welche einstellungen hat das turnier?",
      steps=[("get_status", {"type": "tournament"},
              tournament_book_2e(1, 4, 1, "No tournament is currently running.").replace(
                  "Qapla 0.4.0, Spike 1.4.1, Qapla-baseline", "Qapla 0.4.0, Spike 1.4.1"))],
      reply="Aktuelle Turniereinstellungen:\n"
            + t_settings_book(1, "Das Turnier läuft derzeit nicht."),
      note="Inhaltlich gedeckt. Angehängtes \"Möchtest du etwas ändern?\" entfernt, "
           "Feldbenennung vereinheitlicht."),

    R(64,
      user="draw- und resign entscheidung einschalten, parallelität 10, 1000 spiele in 100 "
           "runden, alle drei engines, turnier starten",
      steps=[
          ("list_installed_engines", {}, ENGINE_LIST),
          ("select_engines", {"engines": ["Qapla 0.4.0", "Spike 1.4.1", "Qapla-baseline"]},
           "Selected: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline."),
          ("configure_tournament",
           {"games": 10, "rounds": 100, "concurrency": 10,
            "draw_mode": "active", "resign_mode": "active"},
           tournament_book_2e(10, 10, 100, "No tournament is currently running.", "active")),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier läuft mit Qapla 0.4.0, Spike 1.4.1 und Qapla-baseline: 10 Spiele pro "
            "Runde über 100 Runden, Concurrency 10, Remis- und Aufgabe-Adjudication aktiv.",
      note="Fünf Anweisungen, alle richtig umgesetzt -- aber configure_tournament schickte "
           "zusätzlich time_control=20+0.01 mit, was gar nicht verlangt war (entfernt). Die "
           "drei configure-Aufrufe zu einem zusammengefasst. Vorgeschaltetes get_status "
           "entfernt; list_installed_engines bleibt, weil \"alle drei engines\" die Namen "
           "voraussetzt. Die Antwort erklärte ungefragt Rund-Robin und die Funktionsweise der "
           "Adjudication -- beides gestrichen."),

    R(65,
      user="zeige mir die sprt einstellunge",
      steps=[("get_status", {"type": "sprt"}, sprt_full(WMTEST, WMTEST, 1, 4, "off"))],
      reply="Aktuelle SPRT-Einstellungen:\n"
            + sprt_settings(WMTEST, WMTEST, 1, 4, "off", "Der SPRT-Test läuft derzeit nicht."),
      note="Werte gedeckt. Entfernt: Klammer-Deutungen und der angehängte \"Hinweis\"-Absatz "
           "mit der Rückfrage nach dem Start."),

    R(66,
      user="übernimm opening file aus dem turnier, die pgn datei aus dem turnier sowie draw und "
           "resign einstellungen aus dem turnier",
      steps=[
          ("get_status", {"type": "tournament"},
           tournament_book_2e(10, 10, 100, "A tournament is currently running.", "active")),
          ("configure_sprt",
           {"openings_file": BOOK, "pgn_file": PGN2, "draw_mode": "active",
            "draw_min_full_moves": 80, "draw_required_consecutive_moves": 20,
            "draw_centipawn_threshold": 20, "resign_mode": "active",
            "resign_required_consecutive_moves": 5, "resign_centipawn_threshold": 500,
            "resign_two_sided": False},
           sprt_full(BOOK, PGN2, 1, 4, "active")),
      ],
      reply="Der SPRT-Test übernimmt jetzt aus dem Turnier: Eröffnungsdatei, PGN-Ausgabedatei "
            "sowie Remis- und Aufgabe-Adjudication (beide aktiv).",
      note="KERNFEHLER: die Werte wurden direkt in configure_sprt getippt, ohne sie je "
           "abzulesen -- \"aus dem Turnier übernehmen\" verlangt zwingend ein get_status "
           "(type=tournament) davor, sonst ist es Raten. Drei configure_sprt-Aufrufe zu einem "
           "zusammengefasst. Die Antwort listete zusätzlich Elo-Grenzen, Max-Spiele und "
           "Zeitkontrolle auf, die kein Tool in diesem Turn gemeldet hat."),

    R(67,
      user="setze die concurrency vom turnier auf 2 und von sprt auf 2",
      steps=[
          ("configure_tournament", {"concurrency": 2},
           tournament_book_2e(2, 10, 100, "A tournament is currently running.", "active")),
          ("configure_sprt", {"concurrency": 2}, sprt_full(BOOK, PGN2, 2, 4, "active")),
      ],
      reply="Die Concurrency steht jetzt bei Turnier und SPRT-Test jeweils auf 2.",
      note="Beide Tools korrekt und getrennt aufgerufen. Nur die angehängte Rückfrage "
           "entfernt."),

    R(68,
      user="gibt es noch etwas im sprt turnier das nicht passt und eingestellt werden sollte?",
      steps=[("get_status", {"type": "sprt"}, sprt_full(BOOK, PGN2, 2, 4, "active"))],
      reply="Auffällig ist nur ein Wert: Max. Spiele steht auf 4. Damit endet der Test nach "
            "vier Partien, lange bevor eine SPRT-Entscheidung fallen kann. Soll ich das "
            "hochsetzen?",
      note="Das richtige Tool wurde aufgerufen, aber die Antwort erfand fünf "
           "Verbesserungsvorschläge mit falschen Begründungen -- unter anderem, eine "
           "Zeitkontrolle von \"5+0 oder 10+0\" sei länger als die eingestellten 60+0 "
           "(sie ist kürzer), und H1=5 sei zu eng. Reduziert auf den einen Punkt, der sich "
           "aus dem Tool-Ergebnis selbst belegen lässt."),

    R(69,
      user="setzte maxgames auf 10000 und starte sprt",
      context=[("gibt es noch etwas im sprt turnier das nicht passt und eingestellt werden "
                "sollte?",
                "Auffällig ist nur ein Wert: Max. Spiele steht auf 4. Damit endet der Test "
                "nach vier Partien, lange bevor eine SPRT-Entscheidung fallen kann. Soll ich "
                "das hochsetzen?")],
      steps=[
          ("configure_sprt", {"max_games": 10000}, sprt_full(BOOK, PGN2, 2, 10000, "active")),
          ("start", {"type": "sprt"}, STARTED_SPRT_BESIDE_T),
      ],
      reply="Max. Spiele steht jetzt auf 10000, der SPRT-Test läuft.",
      note="Tools waren richtig. Denk-Preamble entfernt; die Antwort zählte acht Werte auf, "
           "von denen nur max_games im Tool-Ergebnis dieses Turns stand."),
]
