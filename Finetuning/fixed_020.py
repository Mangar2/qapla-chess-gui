"""Corrected records 20-29."""
from build import R
from fixed_000 import SHOW_TOURNAMENT, tournament_status, STARTED_TOURNAMENT
from fixed_010 import STOP_TOURNAMENT_ABRUPT

EPD_FILE_TESTER = "/Users/volkerbohm/dev/qapla/engine-tester/test/epd/wmtest.epd"
EPD_FILE_GUI = "/Users/volkerbohm/dev/qapla/chess-gui/src/test-system/test-data/wmtest.epd"

STOP_TOURNAMENT_GRACEFUL = "Stopping the tournament gracefully: games already in progress will be finished, no new games will start. Currently: a tournament is running but stopping gracefully (finishing in-progress games)."
STARTED_EPD_ALONE = "EPD analysis started. Currently: an EPD analysis is starting."


def epd_status(concurrency, epd_file, running="No EPD analysis is currently running."):
    return (
        f"Engines: Qapla 0.4.0. EPD file: {epd_file}. Max time per position: 6s. "
        f"Min time per position: 5s. Seen plies (early stop): 2. Concurrency: {concurrency}. "
        f"{running}"
    )


# The settings listing shared by 24/25/27 -- only fields that actually exist in the
# tool result, in the tool result's own order.
def settings_list(pgn, concurrency, draw_min_full_moves, running_line):
    return (
        "- Engines: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline\n"
        "- Zeitkontrolle: 20+0.01\n"
        "- Spiele pro Paarung: 10\n"
        "- Runden: 100\n"
        "- Event-Name: (nicht gesetzt)\n"
        "- Eröffnungsdatei: /Users/volkerbohm/dev/qapla/chess-gui/src/test-system/"
        "test-data/wmtest.epd\n"
        f"- PGN-Ausgabedatei: {pgn}\n"
        f"- Concurrency: {concurrency}\n"
        f"- Remis-Adjudication: aktiv (min. volle Züge {draw_min_full_moves}, "
        "20 aufeinanderfolgende Züge, Schwelle 20 Centipawns)\n"
        "- Aufgabe-Adjudication: aktiv (5 aufeinanderfolgende Züge, Schwelle 600 Centipawns, "
        "nicht zweiseitig)\n"
        f"\n{running_line}"
    )


PGN2 = "/Users/volkerbohm/dev/qapla/chess-gui/log/aitest2.pgn"

RECORDS = [
    R(20,
      user="gib mir das aktuelle spielergebnis",
      steps=[("show_result", {"type": "tournament"}, SHOW_TOURNAMENT)],
      reply="Hier sind die aktuellen Turnierergebnisse.",
      note="Unverändert übernommen -- richtiges Tool, kurze Bestätigung ohne Zahlen."),

    R(21,
      user="bitte starte einn epd test mit qapla 0.4.0 mit 4 parallelen prozessen",
      steps=[
          ("select_epd_engines", {"engines": ["Qapla 0.4.0"]},
           "Selected for EPD analysis: Qapla 0.4.0."),
          ("configure_epd", {"concurrency": 4}, epd_status(4, EPD_FILE_TESTER)),
          ("start", {"type": "epd"}, STARTED_EPD_ALONE),
      ],
      reply="Die EPD-Analyse läuft mit Qapla 0.4.0 und 4 parallelen Prozessen.",
      note="KERNFEHLER: startete nicht, sondern fragte nach der EPD-Datei -- obwohl das "
           "get_status-Ergebnis direkt darüber zeigte, dass eine gültige Datei konfiguriert "
           "ist. Verstößt gegen \"Never ask for something already configured\". Zusätzlich "
           "entfernt: überflüssiges list_installed_engines und get_status."),

    R(22,
      user="ja, bitte nehme die",
      context=[("bitte starte einn epd test mit qapla 0.4.0 mit 4 parallelen prozessen",
                "Ich muss noch eine EPD-Datei konfigurieren, bevor der Test starten kann. "
                f"Soll ich {EPD_FILE_TESTER} verwenden?")],
      steps=[
          ("configure_epd", {"epd_file": EPD_FILE_TESTER}, epd_status(4, EPD_FILE_TESTER)),
          ("start", {"type": "epd"}, STARTED_EPD_ALONE),
      ],
      reply="Die EPD-Analyse läuft mit Qapla 0.4.0 und 4 parallelen Prozessen.",
      note="Kontext mit der alten Rückfrage vorangestellt. Doppeltes select_epd_engines "
           "entfernt (war im Vorgänger-Turn schon gesetzt)."),

    R(23,
      user="bitte starte das turnier, aber mit einer anderen pgn datei",
      steps=[
          ("configure_tournament", {"pgn_file_dialog": True},
           f"PGN output file set to: {PGN2}"),
      ],
      reply=None,
      note="Der Dialog-Aufruf ist terminal (gui-tool-tournament-register.cpp:441) -- der Turn "
           "endet hier, der Start muss im nächsten Turn kommen. Entfernt: vorgeschaltetes "
           "get_status, ein zweiter configure_tournament mit demselben Pfad (der Dialog hatte "
           "ihn bereits gesetzt), sowie start und Antwort, die real nicht mehr stattfinden "
           "konnten."),

    R(24,
      user="bitte nenne mir die turniereinstellungen",
      steps=[("get_status", {"type": "tournament"},
              tournament_status(1, "A tournament is currently running.").replace(
                  "log/aitest.pgn", "log/aitest2.pgn"))],
      reply="Aktuelle Turniereinstellungen:\n" + settings_list(PGN2, 1, 80, "Das Turnier läuft."),
      note="SCHWERE HALLUZINATION: die Antwort erfand acht Einstellungen, die es nicht gibt "
           "(\"Zeichensatz: Unicode\", \"Zufallszug-Modus\", \"Auktionsmodus\", \"Abbruch bei "
           "Zugversagen\", ...). Neu geschrieben, ausschließlich aus den Feldern des "
           "Tool-Ergebnisses."),

    R(25,
      user="du hsat in den turniereinstellungen informationen aufgezählt die es soweit ich weiss "
           "garnicht gibt, z.B. Abbruch bei Zugversagen: keine. Bitte liste noch mal die "
           "Turniereinstellungen aber nur diejenigen, die es wirklich gibt",
      context=[("bitte nenne mir die turniereinstellungen",
                "Hier sind die aktuellen Turniereinstellungen:\n- Engines: Qapla 0.4.0, "
                "Spike 1.4.1, Qapla-baseline\n- Zeichensatz: Unicode\n- Zufallszug-Modus: "
                "keine\n- Auktionsmodus: keine\n- Abbruch bei Zugversagen: keine\n"
                "- Schachmatt-Auslösung: keine")],
      steps=[("get_status", {"type": "tournament"},
              tournament_status(1, "A tournament is currently running.").replace(
                  "log/aitest.pgn", "log/aitest2.pgn"))],
      reply="Aktuelle Turniereinstellungen:\n" + settings_list(PGN2, 1, 80, "Das Turnier läuft."),
      note="Tool und Inhalt waren richtig -- nur deutsch/englisch gemischte Feldnamen "
           "vereinheitlicht. Kontext ist die halluzinierte Liste aus Turn 24, damit das "
           "Record die Korrektur zeigt."),

    R(26,
      user="stoppe das turnier aprupt, setzte die mindestanzahl voller Züge bei der "
           "unentschieden - entscheidung auf 60 und starte das turnier wieder",
      steps=[
          ("stop", {"type": "tournament", "mode": "abrupt"}, STOP_TOURNAMENT_ABRUPT),
          ("configure_tournament", {"draw_min_full_moves": 60},
           tournament_status(1).replace("log/aitest.pgn", "log/aitest2.pgn").replace(
               "min full moves=80", "min full moves=60")),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier wurde abrupt gestoppt, die Mindestanzahl voller Züge für die "
            "Remis-Entscheidung steht jetzt auf 60, und das Turnier läuft wieder.",
      note="Unverändert übernommen -- drei Anweisungen in einer Nachricht, alle drei korrekt "
           "und in der richtigen Reihenfolge ausgeführt, Antwort vollständig gedeckt."),

    R(27,
      user="zeige mir die aktuellen turniereinstellungen",
      steps=[("get_status", {"type": "tournament"},
              tournament_status(1, "A tournament is currently running.").replace(
                  "log/aitest.pgn", "log/aitest2.pgn").replace(
                  "min full moves=80", "min full moves=60"))],
      reply="Aktuelle Turniereinstellungen:\n" + settings_list(PGN2, 1, 60, "Das Turnier läuft."),
      note="Inhaltlich korrekt, nur gemischte Feldnamen vereinheitlicht."),

    R(28,
      user="stoppe das turnier",
      steps=[("stop", {"type": "tournament"}, STOP_TOURNAMENT_GRACEFUL)],
      reply="Das Turnier wird angehalten: laufende Partien werden noch zu Ende gespielt, "
            "neue starten nicht mehr.",
      note="Antwort sagte \"wurde angehalten\", das Tool meldet aber einen laufenden sanften "
           "Stopp. Präzisiert -- der Unterschied ist genau das, wonach der User in Record 14 "
           "gefragt hat."),

    R(29,
      user="bitte starte die epd analyse mit qapla 0.4 und 4 läufen parallel",
      steps=[
          ("select_epd_engines", {"engines": ["qapla 0.4"]},
           "Selected for EPD analysis: Qapla 0.4.0."),
          ("configure_epd", {"concurrency": 4}, epd_status(4, EPD_FILE_GUI)),
          ("start", {"type": "epd"}, STARTED_EPD_ALONE),
      ],
      reply="Die EPD-Analyse läuft mit Qapla 0.4.0 und 4 parallelen Läufen.",
      note="Vorgeschaltetes get_status und Denk-Preamble entfernt. Die alte Antwort nannte "
           "zusätzlich die Datei wmtest.epd -- die stand nur im entfernten get_status, also "
           "aus der Antwort genommen. select_epd_engines ergänzt, weil der User die Engine "
           "ausdrücklich nannte."),
]
