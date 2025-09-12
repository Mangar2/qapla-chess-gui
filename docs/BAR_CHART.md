# ImGui Bar-Chart für Schach-Engine Bewertungen

Diese Implementierung bietet ein konfigurierbares Bar-Chart für die Anzeige von Schach-Engine-Bewertungen in Centipawn über die Zeit (Halbzüge).

## Features

### 🎯 Hauptfunktionen
- **Auto-Skalierung**: Y-Achse skaliert automatisch basierend auf den Datenwerten
- **Konfigurierbare Grenzen**: Min/Max Skalierung von ±200cp bis ±10000cp
- **Achsen-Zeichnung**: X-Achse (Mitte) und Y-Achse (links) mit Beschriftungen
- **Abwechselnde Farben**: Verschiedene Farben für Weiß/Schwarz-Züge
- **Rahmen und Füllung**: Jeder Balken hat Rahmen- und Füllfarben

### 📊 Technische Details
- **X-Achse**: Halbzugnummern (1, 2, 3, ...)
- **Y-Achse**: Centipawn-Werte (-∞ bis +∞)
- **Nulllinie**: Horizontale Linie in der Mitte für ausgeglichene Stellung
- **Gitterlinien**: Horizontale Hilfslinien für bessere Lesbarkeit

## Verwendung

### Basis-Verwendung
```cpp
#include "imgui-barchart.h"

// Erstelle Bar-Chart mit Standard-Konfiguration
QaplaWindows::ImGuiBarChart chart;

// Füge Centipawn-Werte hinzu (Reihenfolge = Halbzug-Reihenfolge)
chart.addValue(15);    // Halbzug 1 (Weiß): +15cp
chart.addValue(-10);   // Halbzug 2 (Schwarz): -10cp  
chart.addValue(120);   // Halbzug 3 (Weiß): +120cp
chart.addValue(-80);   // Halbzug 4 (Schwarz): -80cp

// In ImGui-Schleife zeichnen
if (ImGui::Begin("Engine Evaluation")) {
    chart.draw();
    ImGui::End();
}
```

### Erweiterte Konfiguration
```cpp
// Benutzerdefinierte Konfiguration
QaplaWindows::BarChartConfig config;
config.minScaleCP = 100;      // Min-Skalierung ±100cp
config.maxScaleCP = 2000;     // Max-Skalierung ±2000cp
config.chartWidth = 800.0f;   // Breite
config.chartHeight = 400.0f;  // Höhe
config.barWidth = 10.0f;      // Balkenbreite
config.barSpacing = 3.0f;     // Abstand zwischen Balken

// Farben anpassen
config.whiteBarFill = IM_COL32(255, 255, 220, 180);
config.blackBarFill = IM_COL32(180, 180, 200, 180);

QaplaWindows::ImGuiBarChart chart(config);
```

## Konfiguration

### BarChartConfig Struktur

```cpp
struct BarChartConfig {
    // Skalierung
    int32_t minScaleCP = 200;     // Minimum ±200cp
    int32_t maxScaleCP = 10000;   // Maximum ±10000cp
    
    // Dimensionen
    float chartWidth = 400.0f;
    float chartHeight = 300.0f;
    float barWidth = 8.0f;
    float barSpacing = 2.0f;
    
    // Farben (Weiß-Züge)
    ImU32 whiteBarFill = IM_COL32(220, 220, 255, 180);
    ImU32 whiteBarBorder = IM_COL32(100, 100, 150, 255);
    
    // Farben (Schwarz-Züge)  
    ImU32 blackBarFill = IM_COL32(180, 180, 200, 180);
    ImU32 blackBarBorder = IM_COL32(60, 60, 80, 255);
    
    // UI-Farben
    ImU32 axisColor = IM_COL32(150, 150, 150, 255);
    ImU32 textColor = IM_COL32(200, 200, 200, 255);
    ImU32 gridColor = IM_COL32(100, 100, 100, 100);
};
```

## API Reference

### Hauptmethoden
- `void addValue(int32_t value)` - Fügt einen neuen Centipawn-Wert hinzu
- `void clearValues()` - Löscht alle Werte
- `void draw()` - Zeichnet das Bar-Chart
- `void setConfig(const BarChartConfig& config)` - Setzt neue Konfiguration

### Auto-Skalierung
Die Y-Achse skaliert automatisch basierend auf den Min/Max-Werten:
- **Unter 500cp**: 100cp-Schritte
- **500-2000cp**: 250cp-Schritte  
- **2000-5000cp**: 500cp-Schritte
- **Über 5000cp**: 1000cp-Schritte

### Farb-Schema
- **Weiße Züge** (ungerade Halbzüge): Helle Farben
- **Schwarze Züge** (gerade Halbzüge): Dunkle Farben
- **Positive Werte**: Balken nach oben
- **Negative Werte**: Balken nach unten

## Integration in Schach-GUI

```cpp
class ChessAnalysis {
private:
    QaplaWindows::ImGuiBarChart evaluationChart_;
    
public:
    void onEngineEvaluation(int32_t centipawn) {
        evaluationChart_.addValue(centipawn);
    }
    
    void renderAnalysisWindow() {
        if (ImGui::Begin("Engine Analysis")) {
            ImGui::Text("Position Evaluation");
            evaluationChart_.draw();
            ImGui::End();
        }
    }
};
```

## Beispiel-Werte

Typische Centipawn-Werte:
- **0cp**: Ausgeglichene Stellung
- **±50cp**: Leichter Vorteil
- **±100cp**: Deutlicher Vorteil  
- **±200cp**: Entscheidender Vorteil
- **±500cp**: Gewinnstellung
- **±1000cp+**: Material-Vorteil

## Dateien

- `imgui-barchart.h` - Header mit Klassen-Definition
- `imgui-barchart.cpp` - Implementierung
