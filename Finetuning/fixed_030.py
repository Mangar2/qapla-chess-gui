"""Corrected records 30-39."""
from build import R
from fixed_000 import STARTED_TOURNAMENT
from fixed_010 import STOP_EPD_GRACEFUL, STOP_TOURNAMENT_ABRUPT
from fixed_020 import STOP_TOURNAMENT_GRACEFUL, STARTED_EPD_ALONE, EPD_FILE_GUI

ENGINE_LIST = ('[{"name":"Qapla 0.4.0","protocol":"uci"},{"name":"Spike 1.4.1","protocol":"uci"},'
               '{"name":"Qapla-baseline","protocol":"uci"}]')

# Two engines only, adjudication off -- the state these records were recorded in.
def tournament_status_2e(concurrency, games, rounds, pgn, running):
    return (
        "Engines: Qapla 0.4.0, Spike 1.4.1. Time control: 20+0.01. "
        f"Games per pairing: {games}. Rounds: {rounds}. Event name: (not set). "
        f"Openings file: {EPD_FILE_GUI}. PGN output file: {pgn}. "
        f"Concurrency: {concurrency}. {running} "
        "Draw adjudication: off (min full moves=80, required consecutive moves=20, "
        "centipawn threshold=20). Resign adjudication: off (required consecutive moves=5, "
        "centipawn threshold=500, two-sided=no)."
    )


PGN2 = "/Users/volkerbohm/dev/qapla/chess-gui/log/aitest2.pgn"
BOOK = "/Users/volkerbohm/dev/qapla/engine-tester/test/opening/book8ply.raw"

RECORDS = [
    R(30,
      user="stoppe die epd analyse, füge spike zur analyse hinzu und starte sie wieder",
      steps=[
          ("stop", {"type": "epd"}, STOP_EPD_GRACEFUL),
          ("select_epd_engines", {"engines": ["Qapla 0.4.0", "spike"]},
           "Selected for EPD analysis: Qapla 0.4.0, Spike 1.4.1."),
          ("start", {"type": "epd"},
           "Cannot start the EPD analysis: its settings changed after the previous run "
           "stopped. Call clear_result (type=\"epd\") first, then start again."),
          ("clear_result", {"type": "epd"}, "All EPD analysis results have been cleared."),
          ("start", {"type": "epd"}, STARTED_EPD_ALONE),
      ],
      reply="Die EPD-Analyse läuft wieder, jetzt mit Qapla 0.4.0 und Spike 1.4.1. Die "
            "bisherigen Ergebnisse mussten dafür verworfen werden, weil sich die Auswahl "
            "geändert hat.",
      note="KERNFEHLER: kein Tool-Aufruf, alle drei Anweisungen wurden nur angekündigt. "
           "Neu aufgebaut. select_epd_engines ersetzt die Auswahl, also müssen beide Engines "
           "übergeben werden -- \"hinzufügen\" heißt hier nicht, nur Spike zu schicken. Der "
           "fehlschlagende start plus clear_result folgt der dokumentierten start-Regel für "
           "EPD (geänderte Einstellungen nach einem gestoppten Lauf)."),

    R(31,
      user="stoppe die epd analyse",
      steps=[("stop", {"type": "epd"}, STOP_EPD_GRACEFUL)],
      reply="Die EPD-Analyse wird angehalten: begonnene Stellungen werden noch fertig "
            "analysiert, neue starten nicht mehr.",
      note="Antwort sagte \"wurde gestoppt\", das Tool meldet einen laufenden sanften Stopp. "
           "Präzisiert."),

    R(32,
      user="bitte starte das turnier",
      steps=[("start", {"type": "tournament"}, STARTED_TOURNAMENT)],
      reply="Das Turnier läuft.",
      note="Vorgeschaltetes get_status entfernt -- der Start war direkt verlangt."),

    R(33,
      user="stoppe das turnier",
      steps=[("stop", {"type": "tournament", "mode": "graceful"}, STOP_TOURNAMENT_GRACEFUL)],
      reply="Das Turnier wird angehalten: laufende Partien werden noch zu Ende gespielt, "
            "neue starten nicht mehr.",
      note="Antwort sagte \"wurde gestoppt\", das Tool meldet einen laufenden sanften Stopp. "
           "Präzisiert."),

    R(34,
      user="welche einstellungen hat das turnier aktuell?",
      steps=[("get_status", {"type": "tournament"},
              tournament_status_2e(0, 4, 1, EPD_FILE_GUI,
                                   "No tournament is currently running."))],
      reply="Aktuelle Turniereinstellungen:\n"
            "- Engines: Qapla 0.4.0, Spike 1.4.1\n"
            "- Zeitkontrolle: 20+0.01\n"
            "- Spiele pro Paarung: 4\n"
            "- Runden: 1\n"
            "- Event-Name: (nicht gesetzt)\n"
            f"- Eröffnungsdatei: {EPD_FILE_GUI}\n"
            f"- PGN-Ausgabedatei: {EPD_FILE_GUI}\n"
            "- Concurrency: 0\n"
            "- Remis-Adjudication: aus\n"
            "- Aufgabe-Adjudication: aus\n"
            "\nDas Turnier läuft derzeit nicht.",
      note="Inhaltlich war die Antwort gedeckt. Ergänzt: die beiden Adjudication-Felder, die "
           "im Tool-Ergebnis stehen und in der Antwort fehlten. Format an 24/25/27 angeglichen."),

    R(35,
      user="lass mich das buch und die pgn datei neu setzen",
      steps=[("configure_tournament", {"openings_file_dialog": True},
              f"Openings file set to: {BOOK}")],
      reply=None,
      note="Der Dialog-Aufruf ist terminal -- der Turn endet hier. Der zweite Dialog für die "
           "PGN-Datei und die Antwort konnten real nicht mehr stattfinden, beide entfernt. "
           "Zwei Dateien lassen sich nicht in einem Turn setzen."),

    R(36,
      user="ja, ich möchte 1000 spiele mit 10 spielen pro runde und alle drei engines sollen am "
           "turnier teilnehmen. Concurrency = 10, bitte starten",
      steps=[
          ("list_installed_engines", {}, ENGINE_LIST),
          ("select_engines", {"engines": ["Qapla 0.4.0", "Spike 1.4.1", "Qapla-baseline"]},
           "Selected: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline."),
          ("configure_tournament", {"games": 10, "rounds": 100, "concurrency": 10},
           "Configured: games=10 per round, rounds=100 (1000 games per pairing in total), "
           "concurrency=10."),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier läuft mit Qapla 0.4.0, Spike 1.4.1 und Qapla-baseline: 10 Spiele pro "
            "Runde über 100 Runden, also 1000 pro Paarung, mit Concurrency 10.",
      note="KERNFEHLER: erfand eine Engine namens \"Qapla 0.2.0\", die nie installiert war, "
           "statt bei \"alle drei engines\" die Namen nachzuschlagen -- deshalb ist "
           "list_installed_engines hier richtig und wurde ergänzt. Die alte Antwort erklärte "
           "zudem frei erfunden etwas über Rund-Robin. Die Umrechnung 1000 Spiele / 10 pro "
           "Runde -> rounds=100 war korrekt und bleibt."),

    R(37,
      user="du kannst es auch abrubt beenden, dann stoppt es direkt",
      context=[("stoppe das turnier",
                "Das Turnier wird angehalten: laufende Partien werden noch zu Ende gespielt, "
                "neue starten nicht mehr.")],
      steps=[("stop", {"type": "tournament", "mode": "abrupt"}, STOP_TOURNAMENT_ABRUPT)],
      reply="Das Turnier wurde abrupt beendet, alle laufenden Partien sofort abgebrochen.",
      note="Tool war richtig. Die Antwort kündigte danach an, die installierten Engines zu "
           "prüfen -- eine angekündigte statt ausgeführte Aktion, entfernt."),

    R(38,
      user="ja, mach das",
      context=[("du kannst es auch abrubt beenden, dann stoppt es direkt",
                "Das Turnier wurde abrupt beendet. Ich prüfe nun, welche Engines installiert "
                "sind, um alle für das Turnier auszuwählen.")],
      steps=[
          ("list_installed_engines", {}, ENGINE_LIST),
          ("select_engines", {"engines": ["Qapla 0.4.0", "Spike 1.4.1", "Qapla-baseline"]},
           "Selected: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline."),
          ("configure_tournament", {"games": 10, "rounds": 100, "concurrency": 10},
           "Configured: games=10 per round, rounds=100 (1000 games per pairing in total), "
           "concurrency=10."),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Das Turnier läuft mit Qapla 0.4.0, Spike 1.4.1 und Qapla-baseline: 10 Spiele pro "
            "Runde über 100 Runden, also 1000 pro Paarung, mit Concurrency 10.",
      note="Tool-Kette war richtig. Antwort von Ausrufezeichen und \"Viel Erfolg beim "
           "Turnier!\" befreit. Kontext vorangestellt, damit \"ja, mach das\" einen Bezug hat."),

    R(39,
      user="danke",
      steps=[],
      reply="Gerne!",
      note="Reine Höflichkeitsantwort ohne Tool ist hier richtig. Entfernt wurde der Zusatz "
           "\"Das Turnier läuft nun mit allen drei Engines\" -- in diesem Turn wurde kein Tool "
           "aufgerufen, die Aussage war also durch nichts gedeckt."),
]
