"""Corrected records 110-119."""
from build import R
from fixed_040 import BOOK, PGN2
from fixed_100 import t_status

CANCELLED = ("The user cancelled the dialog; the PGN output file is unchanged: "
             f"{PGN2}")

RECORDS = [
    R(110,
      user="du hast sie nicht im turnier gesetzt",
      context=[("bitte lasse mich die pgn datei im turnier setzen", "")],
      steps=[("get_status", {"type": "tournament"}, t_status(BOOK))],
      reply=f"Doch -- die PGN-Ausgabedatei des Turniers steht auf {PGN2}.",
      note="get_status ist bei einem Widerspruch des Users genau richtig. Denk-Preamble "
           "entfernt und der Zusatz \"Das ist genau die Datei, die Sie gewünscht haben\" -- "
           "was der User wollte, weiß kein Tool."),

    R(111,
      user="ok, dann lass sie mich noch mal auswählen",
      steps=[("configure_tournament", {"pgn_file_dialog": True}, CANCELLED)],
      reply=None,
      note="FALSCHES TOOL: open_pgn_file öffnet eine PGN nur zum Ansehen. Verlangt war erneut "
           "das Setzen der Ausgabedatei -- configure_tournament mit pgn_file_dialog. Auch bei "
           "Abbruch ist der Aufruf terminal (dialogOpened wird unbedingt gesetzt, "
           "gui-tool-tournament-register.cpp:472), der Turn endet also ohne Antwort."),

    R(112,
      user="ja aber die vom TURNIER",
      context=[("ok, dann lass sie mich noch mal auswählen",
                "Die Auswahl wurde abgebrochen. Soll ich die Datei noch einmal öffnen?")],
      steps=[("open_pgn_file", {"source": "tournament"},
              f"Opened PGN file in the Pgn tab: {PGN2}")],
      reply=f"Die PGN-Datei des Turniers ist im Pgn-Tab geöffnet: {PGN2}",
      note="Hier ist open_pgn_file mit source=tournament richtig -- der User bejaht das "
           "Öffnen und präzisiert nur, welche Datei. Antwort sprachlich geglättet."),

    R(113,
      user="nein. ich will den dateinamen der pgn datei im turnier ändern",
      context=[("ja aber die vom TURNIER",
                f"Die PGN-Datei des Turniers ist im Pgn-Tab geöffnet: {PGN2}")],
      steps=[("configure_tournament", {"pgn_file_dialog": True},
              f"PGN output file set to: {PGN2}")],
      reply=None,
      note="Richtiges Tool, terminal, keine Antwort -- war schon korrekt. Denk-Preamble "
           "entfernt, Tool-Ergebnis auf das heutige Format gebracht."),

    R(114,
      user="hast du nicht eben schon gesagt bekommen, dass die pgn datei bereits gesetzt wurde? "
           "Dann darfst du doch nicht sagen der datei dialog wurde geöffnet wo du doch weisst "
           "das der lange wieder zu ist, was ist passiert?",
      context=[("nein. ich will den dateinamen der pgn datei im turnier ändern",
                "Der Dialog zur Auswahl einer PGN-Datei wurde jetzt geöffnet. Bitte wählen Sie "
                "die neue Datei aus.")],
      steps=[],
      reply="Richtig. Der Dialog ist längst wieder zu, wenn ich antworte -- das Tool liefert "
            "sein Ergebnis erst zurück, nachdem du gewählt oder abgebrochen hast. Ihn als "
            "gerade geöffnet anzukündigen war schlicht falsch; ich hätte das Ergebnis melden "
            "müssen.",
      note="KERNFEHLER: auf eine Frage nach dem Warum rief das Modell erneut ein Tool auf "
           "(open_pgn_file) statt zu antworten. Die lange Entschuldigungs-Preamble ist "
           "entfernt, die Erklärung sachlich richtiggestellt."),

    R(115,
      user="ich wollte eine antwort von dir. Du machst etwas falsch ich wollte wissen was ich "
           "dir anders sagen muss, damit du es in zukunft rihtig machst",
      context=[("hast du nicht eben schon gesagt bekommen, dass die pgn datei bereits gesetzt "
                "wurde? was ist passiert?",
                "Meine Entschuldigung - Sie haben absolut recht, ich habe einen Fehler "
                "gemacht. Ich öffne jetzt den Dialog für die PGN-Datei-Auswahl.")],
      steps=[],
      reply="Du musst gar nichts anders sagen -- \"die pgn datei im turnier ändern\" ist "
            "eindeutig. Der Fehler lag bei mir: ich habe einen Dialog als offen beschrieben, "
            "dessen Ergebnis mir bereits vorlag.",
      note="Das Modell rief get_status auf und antwortete mit dem Dateipfad -- die Frage war "
           "aber eine Meta-Frage. Zudem empfahl die Preamble ein Tool namens "
           "\"open_pgn_file_dialog\", das es nicht gibt. Ersetzt durch eine Antwort, die die "
           "Schuld nicht beim User sucht."),

    R(116,
      user="was du falsch gemacht hast ist folgendes zu schreiben \"Der Dialog zur Auswahl "
           "einer PGN-Datei wurde jetzt geöffnet. Bitte wählen Sie die neue Datei aus.\" Ich "
           "möchte wiessen wie ich verhindern kann, dass du das je weider schriebst.",
      steps=[],
      reply="Dagegen musst du nichts tun. Ein Datei-Dialog ist immer schon geschlossen, wenn "
            "ich zu Wort komme -- ich melde deshalb nur noch, was dabei herausgekommen ist, "
            "und fordere dich nie zum Auswählen auf.",
      note="KERNFEHLER: die Antwort war ein leerer String. Ersetzt durch eine Antwort auf die "
           "tatsächlich gestellte Frage."),

    R(117,
      user="und dann machst du den fehler nicht mehr und sagst nicht mehr \"Der Dialog zur "
           "Auswahl einer PGN-Datei wurde jetzt geöffnet\"????",
      context=[("was du falsch gemacht hast ist folgendes zu schreiben \"Der Dialog zur "
                "Auswahl einer PGN-Datei wurde jetzt geöffnet.\" Wie kann ich das verhindern?",
                "")],
      steps=[],
      reply="Ja. Keine Dialog-Ankündigungen mehr, sondern nur noch das Ergebnis -- also "
            "entweder die gesetzte Datei oder der Hinweis, dass abgebrochen wurde.",
      note="Die alte Antwort verlangte vom User, künftig \"change pgn file\" statt \"set pgn "
           "file\" zu sagen -- sie schob das Problem dem User zu, statt es beim Assistenten zu "
           "lassen."),

    R(118,
      user="ändern der turnier-pgn datei",
      steps=[("configure_tournament", {"pgn_file_dialog": True},
              f"PGN output file set to: {PGN2}")],
      reply=None,
      note="KERNFEHLER: das Modell tippte einen Dateinamen, den es sich ausgedacht hatte "
           "(\"aitest2_new.pgn\"). Pfade dürfen nie erfunden werden -- dafür ist der Dialog da. "
           "Vorgeschaltetes get_status entfernt."),

    R(119,
      user="ich möchte die datei ändern",
      steps=[("configure_tournament", {"pgn_file_dialog": True},
              f"PGN output file set to: {PGN2}")],
      reply=None,
      note="War bereits richtig: Dialog statt getipptem Pfad, terminal, keine Antwort. Nur das "
           "Tool-Ergebnis auf das heutige Format gebracht."),
]
