"""Corrected records 0-9."""
from build import R, running, T_RUNNING, S_RUNNING, S_STARTING, S_GRACEFUL, E_RUNNING

OPENINGS = "/Users/volkerbohm/dev/qapla/chess-gui/src/test-system/test-data/wmtest.epd"
TOURNAMENT_PGN = "/Users/volkerbohm/dev/qapla/chess-gui/log/aitest.pgn"

SHOW_TOURNAMENT = (
    "Showing the current tournament results as a table in the chat -- it is already visible "
    "to the user, so do not restate, list, or summarize the numbers in your reply; just "
    "briefly confirm what you did."
)


def sprt_status(concurrency, running="No SPRT test is currently running."):
    return (
        "Champion (comparison engine): Qapla 0.4.0. Challenger (engine under test): Spike 1.4.1. "
        "Time control: 60+0. Elo bounds: H0=0.00, H1=5.00. Alpha=0.050, Beta=0.050. "
        "Max games: 10000. Model: normalized. "
        f"Openings file: {OPENINGS}. PGN output file: (not set). Concurrency: {concurrency}. "
        f"{running} "
        "Draw adjudication: active (min full moves=80, required consecutive moves=20, "
        "centipawn threshold=20). Resign adjudication: active (required consecutive moves=5, "
        "centipawn threshold=500, two-sided=no)."
    )


def tournament_status(concurrency, running="No tournament is currently running."):
    return (
        "Engines: Qapla 0.4.0, Spike 1.4.1, Qapla-baseline. Time control: 20+0.01. "
        "Games per pairing: 10. Rounds: 100. Event name: (not set). "
        f"Openings file: {OPENINGS}. PGN output file: {TOURNAMENT_PGN}. "
        f"Concurrency: {concurrency}. {running} "
        "Draw adjudication: active (min full moves=80, required consecutive moves=20, "
        "centipawn threshold=20). Resign adjudication: active (required consecutive moves=5, "
        "centipawn threshold=600, two-sided=no)."
    )


STARTED_TOURNAMENT = "Tournament started. Currently: a tournament is starting."
STARTED_SPRT_ALONE = "SPRT test started. Currently: an SPRT test is starting."
STARTED_SPRT_BESIDE_TOURNAMENT = "SPRT test started. Currently: a tournament is running; an SPRT test is starting."

RECORDS = [
    R(0,
      user="plz start sprt, current settings, 10 games parallel",
      steps=[
          ("configure_sprt", {"concurrency": 10}, sprt_status(10)),
          ("start", {"type": "sprt"}, STARTED_SPRT_ALONE),
      ],
      reply="SPRT test started with 10 games running in parallel.",
      note="Vorgeschaltetes get_status entfernt (User sagte \"current settings\", also nichts "
           "nachzusehen); Denk-Preambles als Klartext neben den Tool-Calls entfernt."),

    R(1,
      user="ich möchte weitere schach engine hinzufügen",
      steps=[
          ("open_add_engine_dialog", {}, "Added and detected: Qapla-baseline. Ready to use."),
      ],
      reply=None,
      note="open_add_engine_dialog ist terminal (gui-tool-engine-management-register.cpp:104) "
           "-- der Turn endet dort. Das nachfolgende list_installed_engines und die Antwort "
           "konnten real gar nicht stattfinden, beide entfernt."),

    R(2,
      user="bitte starte das turnier",
      steps=[("start", {"type": "tournament"}, STARTED_TOURNAMENT)],
      reply="Das Turnier läuft.",
      note="KERNFEHLER: kein start-Aufruf, stattdessen get_status und eine Rückfrage zur "
           "Concurrency. Ersetzt durch sofortigen start."),

    R(3,
      user="ja, concurrency auf 10 bitte",
      context=[("bitte starte das turnier",
                "Die Concurrency steht auf 0, es würden also keine Spiele parallel laufen. "
                "Soll ich sie erhöhen?")],
      steps=[
          ("configure_tournament", {"concurrency": 10}, tournament_status(10)),
          ("start", {"type": "tournament"}, STARTED_TOURNAMENT),
      ],
      reply="Concurrency steht auf 10, das Turnier läuft.",
      note="Kontext vorangestellt (die alte, bremsende Antwort aus Turn 2) -- erst damit ist "
           "das \"ja\" verständlich und der start hier begründet."),

    R(4,
      user="wie ist der turnierstatus?",
      steps=[("get_status", {"type": "tournament"},
              tournament_status(10, "A tournament is currently running."))],
      reply="Das Turnier läuft: Qapla 0.4.0, Spike 1.4.1 und Qapla-baseline, "
            "Zeitkontrolle 20+0.01, 10 Spiele pro Paarung über 100 Runden, Concurrency 10.",
      note="Antwort war mitten im Wort abgeschnitten (\"Der Turni\"). Neu geschrieben, "
           "ausschließlich aus Werten des Tool-Ergebnisses."),

    R(5,
      user="und das ergebnis?",
      context=[("wie ist der turnierstatus?",
                "Das Turnier läuft: Qapla 0.4.0, Spike 1.4.1 und Qapla-baseline, "
                "Zeitkontrolle 20+0.01, 10 Spiele pro Paarung über 100 Runden, Concurrency 10.")],
      steps=[("show_result", {"type": "tournament"}, SHOW_TOURNAMENT)],
      reply="Die Turnierergebnisse stehen als Tabelle im Chat.",
      note="Überflüssiges get_status vor show_result entfernt; Kontext vorangestellt, damit "
           "sich das \"und\" auf etwas bezieht."),

    R(6,
      user="wie viele spiele müssen noch gespielt werden?",
      steps=[("show_result", {"type": "tournament"}, SHOW_TOURNAMENT)],
      reply="Der aktuelle Stand steht als Tabelle im Chat.",
      note="Überflüssiges get_status entfernt. Antwort nennt bewusst keine Zahlen -- das "
           "Tool-Ergebnis verbietet es ausdrücklich."),

    R(7,
      user="was läuft gerade?",
      steps=[("get_running_status", {}, running(T_RUNNING))],
      reply="Aktuell läuft ein klassisches Turnier, während weder ein SPRT-Test noch eine "
            "EPD-Analyse aktiv sind.",
      note="Unverändert übernommen -- richtiges Tool, Antwort vollständig durch das "
           "Tool-Ergebnis gedeckt."),

    R(8,
      user="kannst du parallel ein sprt starten mit concurrency 1, einstellungen unverändert?",
      steps=[
          ("configure_sprt", {"concurrency": 1}, sprt_status(1)),
          ("start", {"type": "sprt"}, STARTED_SPRT_BESIDE_TOURNAMENT),
      ],
      reply="Der SPRT-Test läuft jetzt mit Concurrency 1, parallel zum Turnier.",
      note="KERNFEHLER: überhaupt kein Tool-Aufruf, stattdessen die Rückfrage, ob ein "
           "Eröffnungsbuch konfiguriert sei -- genau das, was ein Tool beantwortet."),

    R(9,
      user="schau selber nach",
      context=[("kannst du parallel ein sprt starten mit concurrency 1, einstellungen unverändert?",
                "Dafür muss zunächst geprüft werden, ob Engines für den SPRT-Test ausgewählt "
                "sind und ob ein Eröffnungsbuch konfiguriert ist. Ist bereits ein "
                "Eröffnungsbuch für den SPRT konfiguriert?")],
      steps=[
          ("get_status", {"type": "sprt"}, sprt_status(0)),
          ("configure_sprt", {"concurrency": 1}, sprt_status(1)),
          ("start", {"type": "sprt"}, STARTED_SPRT_BESIDE_TOURNAMENT),
      ],
      reply="Champion ist Qapla 0.4.0, Challenger Spike 1.4.1, Eröffnungsbuch wmtest.epd -- "
            "der SPRT-Test läuft jetzt mit Concurrency 1.",
      note="Kontext mit der alten Rückfrage vorangestellt. Damit wird das Record zum "
           "Erholungs-Beispiel: User hakt nach, Assistent sieht selbst nach und handelt."),
]
