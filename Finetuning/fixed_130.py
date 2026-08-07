"""Corrected records 130-139."""
from build import R, running, T_STARTING, T_RUNNING, S_STARTING, S_RUNNING, S_GRACEFUL
from fixed_040 import BOOK, PGN2
from fixed_120 import t3, sprt3

SHOW_EPD = ("Showing the current EPD analysis results as a table in the chat -- it is already "
            "visible to the user, so do not restate, list, or summarize the numbers in your "
            "reply; just briefly confirm what you did. This is the ONLY way you ever learn "
            "which positions were solved or not -- you have no other source for that. Never "
            "state, type, or guess a result yourself instead of calling this; that would be "
            "fabricated information, not a real result.")
SHOW_TOURNAMENT = ("Showing the current tournament results as a table in the chat -- it is "
                   "already visible to the user, so do not restate, list, or summarize the "
                   "numbers in your reply; just briefly confirm what you did.")


def t_list(concurrency, time_control="20.0+0.01"):
    return (
        "- Engines: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline\n"
        f"- Zeitkontrolle: {time_control}\n"
        "- Spiele pro Paarung: 10\n"
        "- Runden: 100\n"
        "- Event-Name: ai-test\n"
        f"- Eröffnungsdatei: {BOOK}\n"
        f"- PGN-Ausgabedatei: {PGN2}\n"
        f"- Concurrency: {concurrency}\n"
        "- Remis-Adjudication: aktiv (min. volle Züge 80, 20 aufeinanderfolgende Züge, "
        "Schwelle 20 Centipawns)\n"
        "- Aufgabe-Adjudication: aktiv (5 aufeinanderfolgende Züge, Schwelle 500 Centipawns, "
        "nicht zweiseitig)\n"
        "\nDas Turnier läuft derzeit nicht."
    )


RECORDS = [
    R(130,
      user="prima, starte das turnier mit 2 spielen parallel",
      steps=[
          ("configure_tournament", {"concurrency": 2}, t3(2, 10, 100, "active")),
          ("start", {"type": "tournament"}, "Tournament started. " + running(T_STARTING)),
      ],
      reply="Das Turnier läuft mit 2 parallelen Partien.",
      note="Tools waren richtig. Denk-Preamble entfernt und der Zusatz \"Bitte warte, bis ich "
           "Ergebnisse zeigen kann\" -- der Assistent kann von sich aus nichts nachliefern."),

    R(131,
      user="was läuft aktuell?",
      steps=[("get_running_status", {}, running(T_RUNNING))],
      reply="Aktuell läuft ein klassisches Turnier.",
      note="Richtiges Tool, aber die Antwort nannte die drei Engine-Namen -- die stehen im "
           "Ergebnis von get_running_status nicht drin. Entfernt."),

    R(132,
      user="starte sprt mit zwei spielen parallel",
      steps=[
          ("configure_sprt", {"concurrency": 2}, sprt3(2)),
          ("start", {"type": "sprt"},
           "SPRT test started. " + running(T_RUNNING, S_STARTING)),
      ],
      reply="Der SPRT-Test läuft mit 2 parallelen Partien, das Turnier läuft weiter.",
      note="Vorgeschaltetes get_status entfernt. Aus der Antwort gestrichen: die Aufzählung "
           "der Einstellungen und der Füllsatz \"Alle anderen Einstellungen wurden "
           "beibehalten\"."),

    R(133,
      user="was läuft?",
      steps=[("get_running_status", {}, running(T_RUNNING, S_RUNNING))],
      reply="Es laufen ein Turnier und ein SPRT-Test.",
      note="Richtiges Tool, aber die Antwort führte Engine-Namen sowie Champion und Challenger "
           "auf -- nichts davon steht im Tool-Ergebnis."),

    R(134,
      user="bitte stoppe sprt nett",
      steps=[("stop", {"type": "sprt", "mode": "graceful"},
              "Stopping the SPRT test gracefully: the game already in progress will be "
              "finished, no new games will start. " + running(T_RUNNING, S_GRACEFUL))],
      reply="Der SPRT-Test wird sanft angehalten: die laufende Partie wird noch beendet. Das "
            "Turnier läuft weiter.",
      note="\"nett\" korrekt als graceful gedeutet. Die Antwort sagte aber \"wurde beendet\" "
           "(der Stopp läuft noch) und sprach von \"3 Engines\", was im Tool-Ergebnis nicht "
           "steht."),

    R(135,
      user="was läuft?",
      steps=[("get_running_status", {}, running(T_RUNNING))],
      reply="Aktuell läuft nur noch das klassische Turnier.",
      note="Der Record, an dem sich die ganze Prüfung entzündet hat: das Tool meldete nur "
           "\"a tournament is running\", die Antwort erfand daraus drei Engine-Namen UND die "
           "Aussage \"Der SPRT-Test wurde bereits beendet\". Beides gestrichen -- dass nur das "
           "Turnier läuft, ergibt sich dagegen zulässig daraus, dass get_running_status "
           "ausschließlich Aktives auflistet."),

    R(136,
      user="was ist das ergebnis vom epd lauf?",
      steps=[("show_result", {"type": "epd"}, SHOW_EPD)],
      reply="Die EPD-Ergebnisse werden als Tabelle im Chat angezeigt.",
      note="Unverändert übernommen -- richtiges Tool, kurze Bestätigung ohne Zahlen."),

    R(137,
      user="Bitte zeige mir die aktuelle turnierkonfiguration",
      steps=[("get_status", {"type": "tournament"}, t3(1, 10, 100, "active"))],
      reply="Aktuelle Turnierkonfiguration:\n" + t_list(1),
      note="Werte gedeckt, aber die Feldnamen entgleist (\"Zugewinnung\", \"20 konsequente "
           "Züge\"). Vereinheitlicht."),

    R(138,
      user="bitte zeige mir das turnierergebnis",
      steps=[("show_result", {"type": "tournament"}, SHOW_TOURNAMENT)],
      reply="Die Turnierergebnisse werden als Tabelle im Chat angezeigt.",
      note="show_result wurde zweimal hintereinander mit identischen Argumenten aufgerufen -- "
           "der zweite Aufruf ist entfernt, ebenso die Denk-Preamble."),

    R(139,
      user="Welche turniereinstellungen habe ich?",
      steps=[("get_status", {"type": "tournament"},
              t3(8, 10, 100, "active").replace("Time control: 20.0+0.01",
                                               "Time control: 60.0+0.0"))],
      reply="Aktuelle Turniereinstellungen:\n" + t_list(8, "60.0+0.0"),
      note="KERNFEHLER aus dem Log von heute: auf eine reine Lesefrage wurde "
           "configure_tournament({concurrency: 8}) aufgerufen -- ein schreibendes Tool, das "
           "die Concurrency tatsächlich verändert hat. Dazu war die Antwort auf Englisch, "
           "unterschlug Qapla-baseline (nannte nur zwei Engines) und deutete die Concurrency "
           "als \"8 engines running in parallel\"."),
]
