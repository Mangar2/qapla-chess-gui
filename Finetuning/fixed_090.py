"""Corrected records 90-99."""
from build import R
from fixed_040 import BOOK, WMTEST, PGN2

STARTED_SPRT_ALONE = "SPRT test started. Currently: an SPRT test is starting."


def sprt_v(openings, pgn, concurrency, max_games,
           running="No SPRT test is currently running.", extra=""):
    return (
        "Champion (comparison engine): Qapla 0.4.0. Challenger (engine under test): Spike 1.4.1. "
        "Time control: 20+0.01. Elo bounds: H0=0.00, H1=5.00. Alpha=0.050, Beta=0.050. "
        f"Max games: {max_games}. Model: normalized. "
        f"Openings file: {openings}. PGN output file: {pgn}. Concurrency: {concurrency}. "
        f"{running} {extra}"
        "Draw adjudication: off (min full moves=80, required consecutive moves=20, "
        "centipawn threshold=20). Resign adjudication: off (required consecutive moves=5, "
        "centipawn threshold=500, two-sided=no)."
    )


def sprt_list(openings, pgn, concurrency, max_games, running_line):
    return (
        "- Champion: Qapla 0.4.0\n"
        "- Challenger: Spike 1.4.1\n"
        "- Zeitkontrolle: 20+0.01\n"
        "- Elo-Grenzen: H0=0.00, H1=5.00\n"
        "- Alpha: 0.050, Beta: 0.050\n"
        f"- Max. Spiele: {max_games}\n"
        "- Modell: normalized\n"
        f"- Eröffnungsdatei: {openings}\n"
        f"- PGN-Ausgabedatei: {pgn}\n"
        f"- Concurrency: {concurrency}\n"
        "- Remis-Adjudication: aus\n"
        "- Aufgabe-Adjudication: aus\n"
        f"\n{running_line}"
    )


DECISION = ('A decision has been reached (or the game limit was hit) -- call show_result '
            '(type="sprt") to see it. ')

RECORDS = [
    R(90,
      user="ja, bitte, concurrency auf 2 und dann starten",
      context=[("bitte zeige mir die sprt konfiguration",
                "Aktuelle SPRT-Konfiguration: Champion Qapla 0.4.0, Challenger Spike 1.4.1, "
                "Zeitkontrolle 20+0.01, Concurrency 1. Der SPRT-Test läuft derzeit nicht.")],
      steps=[
          ("configure_sprt", {"concurrency": 2}, sprt_v(BOOK, PGN2, 2, 10000)),
          ("start", {"type": "sprt"}, STARTED_SPRT_ALONE),
      ],
      reply="Concurrency steht auf 2, der SPRT-Test läuft.",
      note="KERNFEHLER: konfigurierte, startete aber nicht. Die Antwort bestand aus rohem "
           "Tool-Markup als Klartext (\"<tool_call><function=run_test>\") -- ein Tool namens "
           "run_test existiert nicht einmal."),

    R(91,
      user="zeige mir die sprt einstellungen",
      steps=[("get_status", {"type": "sprt"}, sprt_v(BOOK, PGN2, 2, 10000))],
      reply="Aktuelle SPRT-Einstellungen:\n"
            + sprt_list(BOOK, PGN2, 2, 10000, "Der SPRT-Test läuft derzeit nicht."),
      note="KERNFEHLER: kein Tool-Aufruf, und die Antwort war leer (nur zwei Zeilenumbrüche)."),

    R(92,
      user="zeige mir bitte die sprt einstellungen",
      context=[("zeige mir die sprt einstellungen", "")],
      steps=[("get_status", {"type": "sprt"}, sprt_v(WMTEST, PGN2, 1, 4))],
      reply="Aktuelle SPRT-Einstellungen:\n"
            + sprt_list(WMTEST, PGN2, 1, 4, "Der SPRT-Test läuft derzeit nicht."),
      note="Richtiges Tool. Die Antwort verwies den User aber auf \"die Vollantwort von "
           "get_status (type=\\\"sprt\\\") oben\" -- interne Tool-Mechanik, die der User nie "
           "sieht. Concurrency und Adjudication fehlten und wurden ergänzt."),

    R(93,
      user="und wie ist die concurrency?",
      context=[("zeige mir bitte die sprt einstellungen",
                "Aktuelle SPRT-Einstellungen: Champion Qapla 0.4.0, Challenger Spike 1.4.1, "
                "Zeitkontrolle 20+0.01, Max. Spiele 4, Modell normalized.")],
      steps=[("get_status", {"type": "sprt"}, sprt_v(WMTEST, PGN2, 1, 4))],
      reply="Die Concurrency des SPRT-Tests steht auf 1.",
      note="KERNFEHLER: kein Tool-Aufruf, leere Antwort. Jetzt wird genau die gestellte Frage "
           "beantwortet -- ein Wert, nicht der ganze Status."),

    R(94,
      user="setze die concurrency auf 2 und starte sprt",
      steps=[
          ("configure_sprt", {"concurrency": 2}, sprt_v(WMTEST, PGN2, 2, 4)),
          ("start", {"type": "sprt"}, STARTED_SPRT_ALONE),
      ],
      reply="Concurrency steht auf 2, der SPRT-Test läuft.",
      note="Verlangt war ein Feld, geschickt wurden elf -- darunter pgn_file=\"\", was die "
           "PGN-Ausgabedatei gelöscht hätte. Der start-Aufruf trug zudem mode=\"graceful\", ein "
           "Parameter, den start gar nicht hat. Die Antwort war ein leerer String."),

    R(95,
      user="zeige mir die einstellungen von sprt",
      steps=[("get_status", {"type": "sprt"},
              sprt_v(WMTEST, "(not set)", 1, 4, extra=DECISION))],
      reply="Aktuelle SPRT-Einstellungen:\n"
            + sprt_list(WMTEST, "(nicht gesetzt)", 1, 4,
                        "Der SPRT-Test läuft derzeit nicht -- es liegt bereits eine "
                        "Entscheidung vor. Soll ich sie anzeigen?"),
      note="Werte gedeckt, aber die Antwort war auf Englisch, obwohl der User Deutsch schrieb, "
           "und nannte dem User den Tool-Aufruf show_result (type=\"sprt\") zum Selberausführen. "
           "Der Hinweis auf die vorliegende Entscheidung steht im Tool-Ergebnis und bleibt -- "
           "als Angebot, nicht als Tool-Anleitung."),

    R(96,
      user="bitte lass mich die eröffnungdatei setzen",
      steps=[("configure_sprt", {"openings_file_dialog": True},
              f"Openings file set to: {BOOK}")],
      reply=None,
      note="Dialog-Aufruf ist terminal (gui-tool-sprt-register.cpp:175) -- die englische "
           "Antwort samt Rückfrage konnte real nicht stattfinden."),

    R(97,
      user="ja, maxgames bitte auf 10000",
      steps=[("configure_sprt", {"max_games": 10000}, sprt_v(BOOK, "(not set)", 1, 10000))],
      reply="Max. Spiele steht jetzt auf 10000.",
      note="KERNFEHLER: kein Tool-Aufruf, die Änderung wurde behauptet -- und das auf Englisch."),

    R(98,
      user="hast du denn maxgames gesetzt?",
      context=[("ja, maxgames bitte auf 10000",
                "The maximum number of games for the SPRT test has been set to 10000.")],
      steps=[("get_status", {"type": "sprt"}, sprt_v(BOOK, "(not set)", 1, 10000))],
      reply="Ja -- laut aktuellem Status steht Max. Spiele auf 10000.",
      note="KERNFEHLER: auf die Nachfrage kam wortgleich dieselbe Behauptung, wieder ohne Tool. "
           "Bei einer Zweifelsfrage ist genau ein get_status die richtige Antwort."),

    R(99,
      user="setze bitte maxgames von sprt auf 10000",
      steps=[("configure_sprt", {"max_games": 10000}, sprt_v(BOOK, "(not set)", 1, 10000))],
      reply="Max. Spiele steht jetzt auf 10000.",
      note="VOLLSTÄNDIGER ZUSAMMENBRUCH: die Antwort bestand aus dem mehrfach wiederholten "
           "Echo der gesamten bisherigen Unterhaltung, abgeschlossen mit einem Code-Fence. "
           "Kein Tool-Aufruf. Komplett neu geschrieben."),
]
