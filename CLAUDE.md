# Hinweise zur Arbeit an diesem Projekt

Was hier steht, ist nicht aus dem Quelltext ablesbar und muss man wissen, bevor man etwas
ändert.

## Wie ein Bild des UI-Fadens bewertet wird

`UiThreadWatch` misst jedes Bild und meldet eines als Stotterer, das länger als die Schwelle
gebraucht hat. Gezählt wird nur Arbeit: die Drosselung auf feste Bilder pro Sekunde liegt vor
`frameBegin()` und ist gar nicht erst dabei, der Puffertausch und das Warten auf den
GameManager-Pool sind als `Waiting` gemessen und werden abgezogen. Alles andere zählt.

**Im HTTP-Betrieb ist die Schwelle höher, und nur dort.** Ein Aufrufer über HTTP bekommt seine
Antwort auf `start` erst, wenn der Lauf wirklich läuft. Das ist Absicht: früher kam die Antwort
sofort und der Lauf startete nebenher — dann fragte die KI „läuft es?", bekam „nein" und handelte
danach. Das Warten behebt das, und es findet auf dem UI-Faden statt. Einen Lauf zu starten heißt
die Eröffnungsdatei lesen und den Pool hochfahren, gemessen 21 bis 66 ms, und das landet im selben
Bild wie ein Zeichendurchgang von 45 bis 70 ms. Über 100 ms also — von einem Bild, das genau das
tut, was man von ihm verlangt hat.

Darum setzt die Anwendung beim Öffnen der Fernsteuerung `REMOTE_CONTROL_STALL_THRESHOLD`
(300 ms). Das ist kein Freibrief: wonach diese Messung sucht — ein Faden, der von etwas
festgehalten wird, das dort nichts zu suchen hat — ist um Größenordnungen größer. Die Umgebung
`QAPLA_STALL_THRESHOLD_MS` schlägt beides und gehört einem Testläufer.

## Warum die Zeiten so groß aussehen

Der Rechner rendert in Software (`llvmpipe`, Mesa) über xrdp, es gibt keine brauchbare Grafik.
Ein Zeichendurchgang kostet dort im schlechtesten Bild 45 bis 70 ms; im Mittel liegt er unter
2 ms. Ein Messwert von dieser Maschine sagt deshalb nichts über die Leistung der Anwendung auf
einer mit Grafikkarte.

`--remote-desktop` gehört auf jeden Lauf hier: es schaltet VSync ab und lässt das
Holz-Hintergrundbild weg, das sonst in jedem Bild neu gezeichnet wird. Ohne das Flag: gemessen
6 Stotterer, der schlimmste 1876 ms.
