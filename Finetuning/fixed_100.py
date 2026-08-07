"""Corrected records 100-109."""
from build import R
from fixed_040 import BOOK, WMTEST, PGN2
from fixed_090 import sprt_v, STARTED_SPRT_ALONE


def t_status(openings, concurrency=1, running="No tournament is currently running."):
    return (
        "Engines: Qapla 0.4.0, Spike 1.4.1. Time control: 20.0+0.01. Games per pairing: 4. "
        f"Rounds: 1. Event name: ai-test. Openings file: {openings}. "
        f"PGN output file: {PGN2}. Concurrency: {concurrency}. {running} "
        "Draw adjudication: off (min full moves=80, required consecutive moves=20, "
        "centipawn threshold=20). Resign adjudication: off (required consecutive moves=5, "
        "centipawn threshold=500, two-sided=no)."
    )


def t_list(openings, concurrency=1):
    return (
        "- Engines: Qapla 0.4.0, Spike 1.4.1\n"
        "- Zeitkontrolle: 20.0+0.01\n"
        "- Spiele pro Paarung: 4\n"
        "- Runden: 1\n"
        "- Event-Name: ai-test\n"
        f"- Eröffnungsdatei: {openings}\n"
        f"- PGN-Ausgabedatei: {PGN2}\n"
        f"- Concurrency: {concurrency}\n"
        "- Remis-Adjudication: aus\n"
        "- Aufgabe-Adjudication: aus\n"
        "\nDas Turnier läuft derzeit nicht."
    )


RECORDS = [
    R(100,
      user="zeig mir das du das erfolgreich gesetzt hast",
      context=[("setze bitte maxgames von sprt auf 10000",
                "Max. Spiele steht jetzt auf 10000.")],
      steps=[("get_status", {"type": "sprt"}, sprt_v(BOOK, "(not set)", 1, 10000))],
      reply="Laut aktuellem Status steht Max. Spiele auf 10000.",
      note="KERNFEHLER: kein Tool-Aufruf, die Antwort bestand aus einem einzelnen Punkt (\".\"). "
           "Eine Aufforderung, etwas zu belegen, ist genau der Fall für get_status."),

    R(101,
      user="hallo?",
      context=[("zeig mir das du das erfolgreich gesetzt hast", ".")],
      steps=[],
      reply="Ich bin da -- die letzte Antwort war leer, sorry. Was soll ich tun?",
      note="KERNFEHLER: die Antwort war ein leerer String. Eine reine Antwort ohne Tool ist "
           "hier richtig, sie darf nur nicht leer sein und nichts Ungedecktes behaupten."),

    R(102,
      user="setze bitte maxgames im sprt auf 10000",
      steps=[("configure_sprt", {"max_games": 10000}, sprt_v(BOOK, "(not set)", 1, 10000))],
      reply="Max. Spiele im SPRT-Test steht jetzt auf 10000.",
      note="Richtiges Tool. \"erfolgreich\" und die angehängte Rückfrage entfernt."),

    R(103,
      user="nein starte einfach sprt",
      context=[("setze bitte maxgames im sprt auf 10000",
                "Max. Spiele im SPRT-Test steht jetzt auf 10000. Soll ich die aktuelle "
                "SPRT-Konfiguration anzeigen?")],
      steps=[("start", {"type": "sprt"}, STARTED_SPRT_ALONE)],
      reply="Der SPRT-Test läuft.",
      note="Unverändert übernommen -- auf \"starte einfach\" folgt genau ein start, ohne "
           "Vorabprüfung."),

    R(104,
      user="cool, setze bitte concurrency auf 2",
      steps=[("configure_sprt", {"concurrency": 2},
              sprt_v(BOOK, "(not set)", 2, 10000,
                     "An SPRT test is currently running."))],
      reply="Die Concurrency des SPRT-Tests steht jetzt auf 2.",
      note="configure_sprt wurde zweimal mit identischen Argumenten aufgerufen -- der zweite "
           "Aufruf ist entfernt."),

    R(105,
      user="welche einstellungen haben wir in tournament?",
      steps=[("get_status", {"type": "tournament"}, t_status(WMTEST))],
      reply="Aktuelle Turniereinstellungen:\n" + t_list(WMTEST),
      note="Werte gedeckt, aber die Antwort enthielt \"Zugewinnung: off\" als Feldnamen und im "
           "letzten Wort kyrillische Buchstaben (\"momentан\")."),

    R(106,
      user="lass mich die eröffnungdatei ändern",
      steps=[("configure_tournament", {"openings_file_dialog": True},
              f"Openings file set to: {BOOK}")],
      reply=None,
      note="War bereits richtig: Dialog-Aufruf, Turn endet terminal ohne Antwort. Nur das "
           "Tool-Ergebnis vom alten Kurzformat (\"Configured: openings file.\") auf das heutige "
           "gebracht, das den gewählten Pfad zurückmeldet."),

    R(107,
      user="welche einstellungen hat das turnier?",
      steps=[("get_status", {"type": "tournament"}, t_status(BOOK))],
      reply="Aktuelle Turniereinstellungen:\n" + t_list(BOOK),
      note="Werte gedeckt. Entfernt: die Klammer-Deutung der Zeitkontrolle samt dem "
           "verunglückten \"Zug_INCREMENT\", sowie die Feldnamen \"Zug-Prüfung\" und "
           "\"Aufgabe-Prüfung\"."),

    R(108,
      user="lass mich die pgn datei einstellen",
      steps=[("configure_tournament", {"pgn_file_dialog": True},
              f"PGN output file set to: {PGN2}")],
      reply=None,
      note="War bereits richtig (terminal, keine Antwort). Tool-Ergebnis auf das heutige "
           "Format gebracht."),

    R(109,
      user="bitte lasse mich die pgn datei im turnier setzen",
      steps=[("configure_tournament", {"pgn_file_dialog": True},
              f"PGN output file set to: {PGN2}")],
      reply=None,
      note="FALSCHES TOOL: aufgerufen wurde open_pgn_file, das eine PGN-Datei nur zum Ansehen "
           "im Pgn-Tab öffnet. Verlangt war, die PGN-Ausgabedatei des Turniers zu setzen -- "
           "dafür ist configure_tournament mit pgn_file_dialog zuständig. Die alte Antwort "
           "(\"Die PGN-Datei wurde ausgewählt\") verschleierte den Unterschied."),
]
