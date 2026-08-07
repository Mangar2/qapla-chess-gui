"""Corrected records 80-89."""
from build import R, running, T_RUNNING, S_RUNNING, S_STARTING, S_GRACEFUL, E_RUNNING
from fixed_000 import STARTED_TOURNAMENT
from fixed_040 import BOOK, PGN2
from fixed_070 import sprt_tc, tournament_running

EPD_FILE_TESTER = "/Users/volkerbohm/dev/qapla/engine-tester/test/epd/wmtest.epd"

STARTED_EPD_ALL = "EPD analysis started. Currently: a tournament is running; an SPRT test is running; an EPD analysis is starting."
STOP_T_GRACEFUL_ALL = "Stopping the tournament gracefully: games already in progress will be finished, no new games will start. Currently: a tournament is running but stopping gracefully (finishing in-progress games); an SPRT test is running; an EPD analysis is running."
STOP_S_GRACEFUL_ALL = "Stopping the SPRT test gracefully: the game already in progress will be finished, no new games will start. Currently: a tournament is running but stopping gracefully (finishing in-progress games); an SPRT test is running but stopping gracefully (finishing its in-progress game); an EPD analysis is running."
SHOW_SPRT = ("Showing the current SPRT results as tables in the chat -- they are already visible "
             "to the user, so do not restate, list, or summarize the numbers in your reply; just "
             "briefly confirm what you did. This is the ONLY way you ever learn the actual SPRT "
             "decision or duel score -- never state, type, or guess one yourself instead of "
             "calling this.")


def sprt_settings_list(concurrency, running_line, time_control="20+0.01"):
    return (
        "- Champion: Qapla 0.4.0\n"
        "- Challenger: Spike 1.4.1\n"
        f"- Zeitkontrolle: {time_control}\n"
        "- Elo-Grenzen: H0=0.00, H1=5.00\n"
        "- Alpha: 0.050, Beta: 0.050\n"
        "- Max. Spiele: 10000\n"
        "- Modell: normalized\n"
        f"- Eröffnungsdatei: {BOOK}\n"
        f"- PGN-Ausgabedatei: {PGN2}\n"
        f"- Concurrency: {concurrency}\n"
        "- Remis-Adjudication: aktiv (min. volle Züge 80, 20 aufeinanderfolgende Züge, "
        "Schwelle 20 Centipawns)\n"
        "- Aufgabe-Adjudication: aktiv (5 aufeinanderfolgende Züge, Schwelle 500 Centipawns, "
        "nicht zweiseitig)\n"
        f"\n{running_line}"
    )


def t_settings_list(concurrency, event, running_line, time_control="20+0.01"):
    return (
        "- Engines: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline\n"
        f"- Zeitkontrolle: {time_control}\n"
        "- Spiele pro Paarung: 10\n"
        "- Runden: 100\n"
        f"- Event-Name: {event}\n"
        f"- Eröffnungsdatei: {BOOK}\n"
        f"- PGN-Ausgabedatei: {PGN2}\n"
        f"- Concurrency: {concurrency}\n"
        "- Remis-Adjudication: aktiv (min. volle Züge 80, 20 aufeinanderfolgende Züge, "
        "Schwelle 20 Centipawns)\n"
        "- Aufgabe-Adjudication: aktiv (5 aufeinanderfolgende Züge, Schwelle 500 Centipawns, "
        "nicht zweiseitig)\n"
        f"\n{running_line}"
    )


RECORDS = [
    R(80,
      user="Setze in epd seen plies auf 3, min time auf 1 und max time auf 10, starte dann epd "
           "analyse mit zwei parallelen läufen",
      steps=[
          ("configure_epd",
           {"seen_plies": 3, "min_time_seconds": 1, "max_time_seconds": 10, "concurrency": 2},
           f"Engines: Qapla 0.4.0. EPD file: {EPD_FILE_TESTER}. Max time per position: 10s. "
           "Min time per position: 1s. Seen plies (early stop): 3. Concurrency: 2. "
           "No EPD analysis is currently running."),
          ("start", {"type": "epd"}, STARTED_EPD_ALL),
      ],
      reply="Die EPD-Analyse läuft mit 2 parallelen Läufen: min. 1s, max. 10s pro Stellung, "
            "3 Plies zum vorzeitigen Abbruch.",
      note="Vier Anweisungen richtig umgesetzt, aber configure_epd schickte zusätzlich "
           "epd_file mit, was nicht verlangt war (entfernt). In der Antwort war seen_plies "
           "als \"Gefundene Züge: 3\" übersetzt -- sachlich falsch, das sind Plies bis zum "
           "vorzeitigen Abbruch."),

    R(81,
      user="was läuft gerade alles?",
      steps=[("get_running_status", {}, running(T_RUNNING, S_RUNNING, E_RUNNING))],
      reply="Derzeit laufen gleichzeitig ein klassisches Turnier, ein SPRT-Test und eine "
            "EPD-Analyse.",
      note="Unverändert übernommen -- richtiges Tool, Antwort vollständig gedeckt."),

    R(82,
      user="stoppe turnier und sprt sanft",
      steps=[
          ("stop", {"type": "tournament", "mode": "graceful"}, STOP_T_GRACEFUL_ALL),
          ("stop", {"type": "sprt", "mode": "graceful"}, STOP_S_GRACEFUL_ALL),
      ],
      reply="Turnier und SPRT-Test werden sanft angehalten: laufende Partien werden noch zu "
            "Ende gespielt, neue starten nicht mehr.",
      note="Beide Stopps richtig und mit richtigem mode. Die Antwort stand aber in der "
           "Vergangenheit (\"wurden zu Ende geführt\"), obwohl ein sanfter Stopp gerade erst "
           "beginnt -- das ist genau der Unterschied, den der User in Record 14 reklamiert hat."),

    R(83,
      user="zeige mir das sprt ergebnis",
      steps=[("show_result", {"type": "sprt"}, SHOW_SPRT)],
      reply="Die SPRT-Ergebnisse werden angezeigt.",
      note="Unverändert übernommen -- richtiges Tool, kurze Bestätigung ohne eine einzige Zahl, "
           "genau wie das Tool-Ergebnis es verlangt."),

    R(84,
      user="setze sprt fort",
      steps=[("start", {"type": "sprt"},
              "SPRT test started. " + running(S_STARTING, E_RUNNING))],
      reply="Der SPRT-Test läuft wieder.",
      note="Vorgeschaltetes get_status entfernt. \"erfolgreich neu gestartet\" auf die "
           "Tatsache reduziert."),

    R(85,
      user="zeige mir die einstellungen des turniers",
      steps=[("get_status", {"type": "tournament"},
              tournament_running(1, "No tournament is currently running."))],
      reply="Aktuelle Turniereinstellungen:\n"
            + t_settings_list(1, "(nicht gesetzt)", "Das Turnier läuft derzeit nicht."),
      note="Werte gedeckt. Feldnamen waren entgleist (\"Zugaben-Adjudikation\", "
           "\"Niederlegung-Adjudikation\") und wurden vereinheitlicht."),

    R(86,
      user="Setze den namen des events auf ai-test",
      steps=[("configure_tournament", {"event": "ai-test"},
              tournament_running(1, "No tournament is currently running.").replace(
                  "Event name: (not set)", "Event name: ai-test"))],
      reply="Der Event-Name des Turniers steht jetzt auf \"ai-test\".",
      note="Richtiges Tool. Die Antwort begann mit dem englischen \"Done –\" und hatte einen "
           "Genusfehler (\"das Eventname\")."),

    R(87,
      user="Wie ist der aktuelle eventname des turniers?",
      steps=[("get_status", {"type": "tournament"},
              tournament_running(1, "No tournament is currently running.").replace(
                  "Event name: (not set)", "Event name: ai-test").replace(
                  "Time control: 20+0.01", "Time control: 20.0+0.01"))],
      reply="Der Event-Name des Turniers ist \"ai-test\".",
      note="Unverändert übernommen -- vorbildlich: es wird genau die gestellte Frage "
           "beantwortet, nicht der ganze Status ausgeschüttet."),

    R(88,
      user="bitte starte das turnier mit concurrency 2",
      steps=[
          ("configure_tournament", {"concurrency": 2},
           tournament_running(2, "No tournament is currently running.").replace(
               "Event name: (not set)", "Event name: ai-test")),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier läuft mit Concurrency 2.",
      note="Unverändert übernommen -- konfigurieren und starten in einem Turn, ohne Rückfrage. "
           "Die Antwort nennt jetzt zusätzlich die Concurrency, die im Tool-Ergebnis steht."),

    R(89,
      user="bitte zeige mir die sprt konfiguration",
      steps=[("get_status", {"type": "sprt"}, sprt_tc("20+0.01", 1))],
      reply="Aktuelle SPRT-Konfiguration:\n"
            + sprt_settings_list(1, "Der SPRT-Test läuft derzeit nicht."),
      note="Alle Werte waren gedeckt. Die Markdown-Tabelle auf dasselbe Listenformat gebracht "
           "wie die übrigen Einstellungs-Records, und die angehängte Rückfrage entfernt."),
]
