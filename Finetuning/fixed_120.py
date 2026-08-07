"""Corrected records 120-129."""
from build import R, running, T_RUNNING, S_RUNNING, S_STARTING, S_GRACEFUL, E_RUNNING
from fixed_000 import STARTED_TOURNAMENT
from fixed_030 import ENGINE_LIST
from fixed_040 import BOOK, PGN2


def t3(concurrency, games, rounds, adjudication="off",
       running="No tournament is currently running."):
    adj = (f"Draw adjudication: {adjudication} (min full moves=80, required consecutive "
           "moves=20, centipawn threshold=20). "
           f"Resign adjudication: {adjudication} (required consecutive moves=5, "
           "centipawn threshold=500, two-sided=no).")
    return (
        "Engines: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline. Time control: 20.0+0.01. "
        f"Games per pairing: {games}. Rounds: {rounds}. Event name: ai-test. "
        f"Openings file: {BOOK}. PGN output file: {PGN2}. "
        f"Concurrency: {concurrency}. {running} {adj}"
    )


def sprt3(concurrency, running="No SPRT test is currently running."):
    return (
        "Champion (comparison engine): Qapla 0.4.0. Challenger (engine under test): Spike 1.4.1. "
        "Time control: 20+0.01. Elo bounds: H0=0.00, H1=5.00. Alpha=0.050, Beta=0.050. "
        "Max games: 10000. Model: normalized. "
        f"Openings file: {BOOK}. PGN output file: (not set). Concurrency: {concurrency}. "
        f"{running} "
        "Draw adjudication: active (min full moves=80, required consecutive moves=20, "
        "centipawn threshold=20). Resign adjudication: active (required consecutive moves=5, "
        "centipawn threshold=500, two-sided=no)."
    )


RUNNING_T_S = "Currently: a tournament is running; an SPRT test is running."

RECORDS = [
    R(120,
      user="alle drei engines sollen am turnier teilnehmen",
      steps=[
          ("list_installed_engines", {}, ENGINE_LIST),
          ("select_engines", {"engines": ["Qapla 0.4.0", "Spike 1.4.1", "Qapla-baseline"]},
           "Selected: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline."),
      ],
      reply="Qapla 0.4.0, Spike 1.4.1 und Qapla-baseline sind jetzt für das Turnier ausgewählt.",
      note="list_installed_engines ist bei \"alle drei\" richtig -- die Namen müssen von "
           "irgendwoher kommen. Das nachgeschaltete get_status trug nichts bei und ist raus."),

    R(121,
      user="bitte setze concurrency auf 2, 1000 spiele in 100 runden und starte das turnier",
      steps=[
          ("configure_tournament", {"concurrency": 2, "games": 10, "rounds": 100},
           t3(2, 10, 100)),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier läuft: 10 Spiele pro Runde über 100 Runden, also 1000 pro Paarung, "
            "mit Concurrency 2.",
      note="Der erste configure-Aufruf vergaß rounds, wurde per get_status geprüft und mit "
           "einem zweiten Aufruf nachgebessert -- zu einem Aufruf zusammengefasst. "
           "Denk-Preamble entfernt. Die Antwort nannte zusätzlich die PGN-Datei, die nur im "
           "gestrichenen get_status stand."),

    R(122,
      user="übernimm die opening datei des turniers auch in sprt",
      steps=[
          ("get_status", {"type": "tournament"}, t3(1, 10, 100)),
          ("get_status", {"type": "sprt"}, sprt3(1)),
      ],
      reply=f"Der SPRT-Test verwendet bereits dieselbe Eröffnungsdatei wie das Turnier: {BOOK}. "
            "Es war nichts zu ändern.",
      note="Vorbildlich: beide Seiten abgelesen, festgestellt, dass die Werte schon "
           "übereinstimmen, und deshalb nichts geändert. Nur sprachlich geglättet."),

    R(123,
      user="bitte aktiviere die unentschieden und aufgabe entscheidung des turniers",
      steps=[("configure_tournament", {"draw_mode": "active", "resign_mode": "active"},
              t3(1, 10, 100, "active"))],
      reply="Remis- und Aufgabe-Adjudication des Turniers sind jetzt aktiv.",
      note="Richtiges Tool, beide Felder in einem Aufruf. Entfernt: die Denk-Preamble, die "
           "erfundenen Begriffe \"Aufgabenzahl\"/\"Unentschiedenzahl\" und die angehängte "
           "Rückfrage nach dem Start."),

    R(124,
      user="ja bittte mit concurrency 2 starten",
      context=[("bitte aktiviere die unentschieden und aufgabe entscheidung des turniers",
                "Remis- und Aufgabe-Adjudication des Turniers sind jetzt aktiv. Möchten Sie "
                "das Turnier jetzt starten?")],
      steps=[
          ("configure_tournament", {"concurrency": 2}, t3(2, 10, 100, "active")),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier läuft mit Concurrency 2.",
      note="configure_tournament schickte sieben Felder mit, obwohl nur die Concurrency neu "
           "war -- Remis und Aufgabe waren im Vorgänger-Turn längst gesetzt. Auf das eine "
           "verlangte Feld reduziert."),

    R(125,
      user="bitte starte auch sprt mit 2 parallelen spielen",
      steps=[
          ("configure_sprt", {"concurrency": 2}, sprt3(2)),
          ("start", {"type": "sprt"},
           "SPRT test started. " + running(T_RUNNING, S_STARTING)),
      ],
      reply="Der SPRT-Test läuft mit 2 parallelen Partien: Qapla 0.4.0 als Champion gegen "
            "Spike 1.4.1 als Challenger.",
      note="Vorgeschaltetes get_status entfernt -- configure_sprt meldet den vollen Status "
           "ohnehin zurück, Champion und Challenger sind dadurch weiter gedeckt."),

    R(126,
      user="was läuft aktuell?",
      steps=[("get_running_status", {}, RUNNING_T_S)],
      reply="Es laufen ein klassisches Turnier und ein SPRT-Test. Eine EPD-Analyse läuft nicht.",
      note="Unverändert übernommen -- richtiges Tool, Antwort gedeckt."),

    R(127,
      user="bitte beende sprt sanft",
      steps=[("stop", {"type": "sprt", "mode": "graceful"},
              "Stopping the SPRT test gracefully: the game already in progress will be "
              "finished, no new games will start. " + running(T_RUNNING, S_GRACEFUL))],
      reply="Der SPRT-Test wird sanft angehalten: die laufende Partie wird noch beendet, neue "
            "starten nicht mehr.",
      note="Richtiges Tool mit richtigem mode. Die Antwort sagte \"erfolgreich sanft "
           "abgebrochen\" -- abgebrochen ist das Gegenteil von sanft, und der Vorgang läuft "
           "noch."),

    R(128,
      user="was läuft aktuell?",
      steps=[("get_running_status", {}, RUNNING_T_S)],
      reply="Es laufen ein klassisches Turnier und ein SPRT-Test. Eine EPD-Analyse läuft nicht.",
      note="Unverändert übernommen. Bemerkenswert: derselbe Status wie in 126, obwohl "
           "dazwischen ein sanfter Stopp lief -- die Antwort bleibt trotzdem beim "
           "Tool-Ergebnis und spekuliert nicht."),

    R(129,
      user="zeige mir die einstellungen vom tunrier",
      steps=[("get_status", {"type": "tournament"}, t3(1, 10, 100, "active"))],
      reply="Aktuelle Turniereinstellungen:\n"
            "- Engines: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline\n"
            "- Zeitkontrolle: 20.0+0.01\n"
            "- Spiele pro Paarung: 10\n"
            "- Runden: 100\n"
            "- Event-Name: ai-test\n"
            f"- Eröffnungsdatei: {BOOK}\n"
            f"- PGN-Ausgabedatei: {PGN2}\n"
            "- Concurrency: 1\n"
            "- Remis-Adjudication: aktiv (min. volle Züge 80, 20 aufeinanderfolgende Züge, "
            "Schwelle 20 Centipawns)\n"
            "- Aufgabe-Adjudication: aktiv (5 aufeinanderfolgende Züge, Schwelle 500 "
            "Centipawns, nicht zweiseitig)\n"
            "\nDas Turnier läuft derzeit nicht.",
      note="Werte gedeckt, aber die Feldnamen entgleist (\"Zugewertung\", \"benötigte "
           "hintereinander gehende Züge\", \"Zentipawn\"). Vereinheitlicht."),
]
