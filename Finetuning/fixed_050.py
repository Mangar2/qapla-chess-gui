"""Corrected records 50-59."""
from build import R
from fixed_000 import STARTED_TOURNAMENT
from fixed_030 import tournament_status_2e
from fixed_040 import sprt, BOOK, WMTEST, PGN2

STARTED_SPRT_ALONE = "SPRT test started. Currently: an SPRT test is starting."


def t_settings(openings, pgn, concurrency, running_line):
    """German settings listing for the two-engine tournament, adjudication off."""
    return (
        "- Engines: Qapla 0.4.0, Spike 1.4.1\n"
        "- Zeitkontrolle: 20+0.01\n"
        "- Spiele pro Paarung: 4\n"
        "- Runden: 1\n"
        "- Event-Name: (nicht gesetzt)\n"
        f"- Eröffnungsdatei: {openings}\n"
        f"- PGN-Ausgabedatei: {pgn}\n"
        f"- Concurrency: {concurrency}\n"
        "- Remis-Adjudication: aus\n"
        "- Aufgabe-Adjudication: aus\n"
        f"\n{running_line}"
    )


RECORDS = [
    R(50,
      user="bitte starte ein sprt turnier",
      steps=[("start", {"type": "sprt"}, STARTED_SPRT_ALONE)],
      reply="Der SPRT-Test läuft.",
      note="Vorgeschaltetes get_status entfernt. Damit entfielen auch Champion und Challenger "
           "aus der Antwort -- sie standen nur in diesem get_status, im start-Ergebnis nicht."),

    R(51,
      user="setze die concurrency des sprt turniers auf 4",
      steps=[("configure_sprt", {"concurrency": 4}, sprt(BOOK, WMTEST, 4))],
      reply="Die Concurrency des SPRT-Tests steht jetzt auf 4.",
      note="Richtiges Tool, Antwort gedeckt. Nur sprachlich geglättet."),

    R(52,
      user="welhe einstellungen hat der sprt lauf?",
      steps=[("get_status", {"type": "sprt"},
              sprt(BOOK, WMTEST, 4, "An SPRT test is currently running."))],
      reply="Aktuelle SPRT-Einstellungen:\n"
            "- Champion: Qapla 0.4.0\n"
            "- Challenger: Spike 1.4.1\n"
            "- Zeitkontrolle: 60+0\n"
            "- Elo-Grenzen: H0=0.00, H1=5.00\n"
            "- Alpha: 0.050, Beta: 0.050\n"
            "- Max. Spiele: 10000\n"
            "- Modell: normalized\n"
            f"- Eröffnungsdatei: {BOOK}\n"
            f"- PGN-Ausgabedatei: {WMTEST}\n"
            "- Concurrency: 4\n"
            "- Remis-Adjudication: aus\n"
            "- Aufgabe-Adjudication: aus\n"
            "\nDer SPRT-Test läuft.",
      note="Werte waren gedeckt, aber mit eigenen Deutungen angereichert: \"(Challenger muss "
           "mindestens 5 Punkte besser sein)\" ist sachlich falsch (H1 ist eine Elo-Schranke, "
           "keine Punktforderung). Alle Klammer-Erklärungen entfernt, fehlende "
           "Adjudication-Felder ergänzt."),

    R(53,
      user="was sind die aktuellen einstellungen vom turnier",
      steps=[("get_status", {"type": "tournament"},
              tournament_status_2e(0, 4, 1, PGN2, "No tournament is currently running."))],
      reply="Aktuelle Turniereinstellungen:\n"
            + t_settings(WMTEST, PGN2, 0, "Das Turnier läuft derzeit nicht."),
      note="Inhaltlich gedeckt. Erfundene Feldnamen wie \"Zugverlaufsbewertung\" durch die "
           "tatsächlichen Bezeichnungen ersetzt."),

    R(54,
      user="die eröffnungsdatei funktioniert nicht, die muss ich neu setzen",
      steps=[("configure_tournament", {"openings_file_dialog": True},
              f"Openings file set to: {BOOK}")],
      reply=None,
      note="Dialog-Aufruf ist terminal -- Turn endet hier. Denk-Preamble und die anschließende "
           "Antwort (samt Rückfrage \"Möchten Sie die Analyse jetzt starten?\") entfernt, "
           "beide konnten real nicht stattfinden."),

    R(55,
      user="bitte stelle das turnier auf 1000 spiele in 100 runden",
      steps=[("configure_tournament", {"games": 10, "rounds": 100},
              "Configured: games=10 per round, rounds=100 "
              "(1000 games per pairing in total).")],
      reply="Das Turnier steht jetzt auf 10 Spiele pro Runde über 100 Runden, also 1000 Spiele "
            "pro Paarung.",
      note="KERNFEHLER: kein Tool-Aufruf, stattdessen die Ankündigung, erst die Eröffnungsdatei "
           "prüfen zu müssen -- eine erfundene Vorbedingung. Die Umrechnung 1000 Spiele / "
           "100 Runden -> games=10 ist genau der im Tool-Schema beschriebene Fall."),

    R(56,
      user="stelle das turnier auf 1000 spiele in 100 runden. prüfe nichts",
      context=[("bitte stelle das turnier auf 1000 spiele in 100 runden",
                "Ich muss prüfen, ob die Eröffnungsdatei korrekt ist, bevor ich die "
                "Einstellungen für das Turnier anpassen kann.")],
      steps=[("configure_tournament", {"games": 10, "rounds": 100},
              "Configured: games=10 per round, rounds=100 "
              "(1000 games per pairing in total).")],
      reply="Das Turnier steht jetzt auf 10 Spiele pro Runde über 100 Runden, also 1000 Spiele "
            "pro Paarung.",
      note="Rechnung und Tool waren richtig. Angehängte Rückfrage \"Möchten Sie, dass ich das "
           "Turnier starte?\" entfernt. Kontext ist die Verweigerung aus Turn 55."),

    R(57,
      user="ja, starte das turnier mit 10 spielen parallel",
      context=[("stelle das turnier auf 1000 spiele in 100 runden. prüfe nichts",
                "Das Turnier steht jetzt auf 10 Spiele pro Runde über 100 Runden, also 1000 "
                "Spiele pro Paarung. Möchten Sie, dass ich das Turnier starte?")],
      steps=[
          ("configure_tournament", {"concurrency": 10},
           tournament_status_2e(10, 10, 100, PGN2, "No tournament is currently running.")),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier läuft mit 10 parallelen Partien: 10 Spiele pro Runde über 100 Runden.",
      note="Denk-Preamble entfernt. games und rounds wurden erneut mitgeschickt, obwohl nur die "
           "Concurrency verlangt war -- auf configure_tournament({concurrency}) reduziert."),

    R(58,
      user="welche einstellungen hat das turnier?",
      steps=[("get_status", {"type": "tournament"},
              tournament_status_2e(0, 4, 1, WMTEST, "No tournament is currently running."))],
      reply="Aktuelle Turniereinstellungen:\n"
            + t_settings(WMTEST, WMTEST, 0, "Das Turnier läuft derzeit nicht."),
      note="Inhaltlich gedeckt, aber mit eigenen Deutungen (\"20 Sekunden ohne Zugzuwachs\") "
           "und einem Sachfehler: die Resign-Bedingung wurde als \"min. Züge=5\" ausgegeben, "
           "gemeint sind 5 aufeinanderfolgende Züge. Vereinheitlicht."),

    R(59,
      user="wie sind die turniereinstellungen?",
      steps=[("get_status", {"type": "tournament"},
              tournament_status_2e(1, 4, 1, WMTEST, "No tournament is currently running."))],
      reply="Aktuelle Turniereinstellungen:\n"
            + t_settings(WMTEST, WMTEST, 1, "Das Turnier läuft derzeit nicht."),
      note="Inhaltlich gedeckt, aber sprachlich entgleist (\"Zugsentschiednung\", "
           "\"Aufgegebene-Zugs-Entscheidung\", \"Zentipawn\"). Auf dieselbe Feldbenennung "
           "gebracht wie 53 und 58."),
]
