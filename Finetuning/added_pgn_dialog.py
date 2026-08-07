"""Neu geschriebene Records -- NICHT aus den Rohdaten korrigiert.

Die Rohdaten enthalten keinen einzigen Turn, der open_pgn_file mit source="dialog"
korrekt behandelt: dort wurde entweder das falsche Tool genommen (open_pgn_file, wo die
PGN-Ausgabedatei gesetzt werden sollte) oder nach dem Dialog noch geantwortet. Seit
gui-tool-app-register.cpp den dialog-Zweig terminal macht, ist das nicht mehr möglich --
gedeckt ist der Fall im Datensatz damit trotzdem nicht.

Beide Records sind erfunden. Sie tragen deshalb ein eigenes Präfix, damit im Verzeichnis
sichtbar bleibt, was aus echten Sitzungen stammt (fixed_*.py) und was nicht.
"""
from build import R

PGN2 = "/Users/volkerbohm/dev/qapla/chess-gui/log/aitest2.pgn"

RECORDS = [
    R(149,
      user="öffne bitte eine pgn datei",
      steps=[("open_pgn_file", {"source": "dialog"},
              f"Opened PGN file in the Pgn tab: {PGN2}")],
      reply=None,
      note="NEU GESCHRIEBEN. Ohne Hinweis auf Turnier oder SPRT ist source=\"dialog\" richtig "
           "-- der User sucht die Datei selbst aus. Der Aufruf ist terminal, der Turn endet "
           "am Tool-Ergebnis, keine Antwort danach."),

    R(150,
      user="ich möchte mir eine pgn datei ansehen, lass mich eine aussuchen",
      steps=[("open_pgn_file", {"source": "dialog"},
              "The user cancelled the dialog; no PGN file was opened.")],
      reply=None,
      note="NEU GESCHRIEBEN. Gegenstück zu 149: auch der Abbruch ist terminal. Genau hier hat "
           "das Modell früher \"bitte wählen Sie nun eine Datei\" geschrieben, obwohl der "
           "Dialog längst zu war."),
]
