/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "imgui-barchart.h"
#include <sstream>
#include <iomanip>

namespace QaplaWindows {

void ImGuiBarChart::calculateScale(int32_t& minValue, int32_t& maxValue, int32_t& scale) const {
    if (values_.empty()) {
        minValue = -config_.minScaleCP;
        maxValue = config_.minScaleCP;
        scale = config_.minScaleCP * 2;
        return;
    }
    
    // Finde Min/Max Werte
    auto minMaxPair = std::minmax_element(values_.begin(), values_.end());
    int32_t dataMin = *minMaxPair.first;
    int32_t dataMax = *minMaxPair.second;
    
    // Berechne benötigte Skalierung
    int32_t maxAbsValue = std::max(std::abs(dataMin), std::abs(dataMax));
    
    // Wende Min/Max Skalierungsgrenzen an
    if (maxAbsValue < config_.minScaleCP) {
        maxAbsValue = config_.minScaleCP;
    } else if (maxAbsValue > config_.maxScaleCP) {
        maxAbsValue = config_.maxScaleCP;
    }
    
    // Runde auf schöne Werte (100er, 500er, 1000er Schritte)
    int32_t niceScale;
    if (maxAbsValue <= 500) {
        niceScale = ((maxAbsValue + 99) / 100) * 100;
    } else if (maxAbsValue <= 2000) {
        niceScale = ((maxAbsValue + 249) / 250) * 250;
    } else if (maxAbsValue <= 5000) {
        niceScale = ((maxAbsValue + 499) / 500) * 500;
    } else {
        niceScale = ((maxAbsValue + 999) / 1000) * 1000;
    }
    
    minValue = -niceScale;
    maxValue = niceScale;
    scale = niceScale * 2;
}

ImVec2 ImGuiBarChart::valueToPixel(float moveIndex, int32_t value, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const {
    float x = chartMin.x + moveIndex * (config_.barWidth + config_.barSpacing);
    
    // Y-Koordinate: 0 ist in der Mitte des Charts
    float yNormalized = static_cast<float>(value - minValue) / static_cast<float>(scale);
    float y = chartMax.y - yNormalized * (chartMax.y - chartMin.y);
    
    return ImVec2(x, y);
}

void ImGuiBarChart::drawXAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const {
    // X-Achse in der Mitte des Charts zeichnen (bei Wert 0)
    float zeroY = chartMax.y - (static_cast<float>(0 - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
    
    drawList->AddLine(
        ImVec2(chartMin.x - 5, zeroY),  // Beginne bei der Y-Achse, nicht 10 Pixel davor
        ImVec2(chartMax.x + 10, zeroY),
        config_.axisColor,
        2.0f
    );
    
    // Halbzug-Markierungen und Labels
    for (size_t i = 0; i < values_.size(); ++i) {
        float x = chartMin.x + i * (config_.barWidth + config_.barSpacing) + config_.barWidth * 0.5f;
        
        // Kleine Markierung
        drawList->AddLine(
            ImVec2(x, zeroY - 3),
            ImVec2(x, zeroY + 3),
            config_.axisColor,
            1.0f
        );
        
        // Label jeden 5. Halbzug
        if (i % 5 == 0 || i == values_.size() - 1) {
            std::string label = std::to_string(i + 1);
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            drawList->AddText(
                ImVec2(x - textSize.x * 0.5f, zeroY + 8),
                config_.textColor,
                label.c_str()
            );
        }
    }
}

void ImGuiBarChart::drawYAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t maxValue, int32_t scale) const {
    // Y-Achse ganz links zeichnen
    drawList->AddLine(
        ImVec2(chartMin.x, chartMin.y - 10),
        ImVec2(chartMin.x, chartMax.y + 10),
        config_.axisColor,
        2.0f
    );
    
    // Berechne Anzahl der Unterteilungen basierend auf verfügbarem Platz
    float chartHeight = chartMax.y - chartMin.y;
    float textHeight = ImGui::GetTextLineHeight();
    float minLabelSpacing = textHeight * 1.5f; // Mindestabstand zwischen Labels
    
    // Maximale Anzahl Labels, die in den verfügbaren Platz passen
    int maxLabels = static_cast<int>(chartHeight / minLabelSpacing);
    maxLabels = std::max(3, std::min(maxLabels, 15)); // Zwischen 3 und 15 Labels
    
    // Berechne ideale Schrittgröße basierend auf der Anzahl der Labels
    int32_t idealStepSize = scale / (maxLabels - 1); // -1 weil wir Min und Max einschließen
    
    // Runde auf "schöne" Werte
    int32_t stepSize;
    if (idealStepSize <= 25) stepSize = 25;
    else if (idealStepSize <= 50) stepSize = 50;
    else if (idealStepSize <= 100) stepSize = 100;
    else if (idealStepSize <= 250) stepSize = 250;
    else if (idealStepSize <= 500) stepSize = 500;
    else if (idealStepSize <= 1000) stepSize = 1000;
    else if (idealStepSize <= 2500) stepSize = 2500;
    else if (idealStepSize <= 5000) stepSize = 5000;
    else stepSize = 10000;
    
    // Zeichne Gitterlinien und Labels symmetrisch um die Nulllinie
    // Bei sehr kleinen Fenstern: Spezialbehandlung für Pixel-Abstand
    bool showZero = maxLabels > 3;
    
    // Sonderfall: Fenster zu klein für 3 Labels
    if (!showZero) {
        // Berechne welche Schrittgröße tatsächlich ins Fenster passt
        // Wir wollen genau 2 Labels: einen positiven und einen negativen Wert
        // Die müssen mindestens minLabelSpacing voneinander entfernt sein
        
        // Verfügbarer Platz für 2 Labels mit Abstand
        float availableSpacingForTwo = chartHeight / 2.0f; // Hälfte für +, Hälfte für -
        
        // Berechne maximalen Wert, der bei availableSpacingForTwo Pixel von der Mitte liegt
        int32_t maxValueThatFits = static_cast<int32_t>((availableSpacingForTwo / chartHeight) * scale);
        
        // Finde eine "schöne" Schrittgröße, die kleiner oder gleich maxValueThatFits ist
        int32_t adjustedStepSize = stepSize;
        while (adjustedStepSize > maxValueThatFits && adjustedStepSize > 25) {
            if (adjustedStepSize >= 1000) adjustedStepSize /= 2;
            else if (adjustedStepSize >= 500) adjustedStepSize = 250;
            else if (adjustedStepSize >= 250) adjustedStepSize = 100;
            else if (adjustedStepSize >= 100) adjustedStepSize = 50;
            else adjustedStepSize = 25;
        }
        stepSize = adjustedStepSize;
    }
    
    if (showZero) {
        // Zeige die Nulllinie (bei größeren Fenstern)
        int32_t value = 0;
        float y = chartMax.y - (static_cast<float>(value - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
        
        // Markierung auf Y-Achse (keine Gitterlinie für 0, wird von X-Achse gemacht)
        drawList->AddLine(
            ImVec2(chartMin.x - 3, y),
            ImVec2(chartMin.x + 3, y),
            config_.axisColor,
            1.0f
        );
        
        // Label
        std::string label = "0";
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        drawList->AddText(
            ImVec2(chartMin.x - textSize.x - 8, y - textSize.y * 0.5f),
            config_.textColor,
            label.c_str()
        );
    }
    
    // Dann positive und negative Werte symmetrisch
    int32_t maxSteps = showZero ? (maxLabels - 1) / 2 : maxLabels / 2;
    
    // Bei sehr kleinen Fenstern: mindestens einen Schritt zeigen
    if (!showZero && maxSteps == 0) {
        maxSteps = 1;
    }
    
    for (int32_t step = 1; step <= maxSteps; ++step) {
        int32_t positiveValue = step * stepSize;
        int32_t negativeValue = -step * stepSize;
        
        // Positive Werte (oberhalb der Nulllinie)
        // Zeige auch Werte die größer als maxValue sind, aber innerhalb der Skalierung liegen
        if (positiveValue <= maxValue || (!showZero && step == 1)) {
            float y = chartMax.y - (static_cast<float>(positiveValue - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
            
            // Gitterlinie
            drawList->AddLine(
                ImVec2(chartMin.x, y),
                ImVec2(chartMax.x, y),
                config_.gridColor,
                1.0f
            );
            
            // Markierung auf Y-Achse
            drawList->AddLine(
                ImVec2(chartMin.x - 3, y),
                ImVec2(chartMin.x + 3, y),
                config_.axisColor,
                1.0f
            );
            
            // Label
            std::string label = std::to_string(positiveValue);
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            drawList->AddText(
                ImVec2(chartMin.x - textSize.x - 8, y - textSize.y * 0.5f),
                config_.textColor,
                label.c_str()
            );
        }
        
        // Negative Werte (unterhalb der Nulllinie)
        // Zeige auch Werte die kleiner als minValue sind, aber innerhalb der Skalierung liegen
        if (negativeValue >= minValue || (!showZero && step == 1)) {
            float y = chartMax.y - (static_cast<float>(negativeValue - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
            
            // Gitterlinie
            drawList->AddLine(
                ImVec2(chartMin.x, y),
                ImVec2(chartMax.x, y),
                config_.gridColor,
                1.0f
            );
            
            // Markierung auf Y-Achse
            drawList->AddLine(
                ImVec2(chartMin.x - 3, y),
                ImVec2(chartMin.x + 3, y),
                config_.axisColor,
                1.0f
            );
            
            // Label
            std::string label = std::to_string(negativeValue);
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            drawList->AddText(
                ImVec2(chartMin.x - textSize.x - 8, y - textSize.y * 0.5f),
                config_.textColor,
                label.c_str()
            );
        }
    }
}

void ImGuiBarChart::drawBars(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const {
    // Berechne Y-Position der Nulllinie
    float zeroY = chartMax.y - (static_cast<float>(0 - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
    
    for (size_t i = 0; i < values_.size(); ++i) {
        int32_t value = values_[i];
        
        // Bestimme Farben (abwechselnd für Weiß/Schwarz)
        bool isWhiteMove = (i % 2 == 0);
        ImU32 fillColor = isWhiteMove ? config_.whiteBarFill : config_.blackBarFill;
        ImU32 borderColor = isWhiteMove ? config_.whiteBarBorder : config_.blackBarBorder;
        
        // Berechne Bar-Position
        float barX = chartMin.x + i * (config_.barWidth + config_.barSpacing);
        ImVec2 valuePixel = valueToPixel(static_cast<float>(i), value, chartMin, chartMax, minValue, scale);
        
        // Bestimme Bar-Rechteck
        ImVec2 barMin, barMax;
        if (value >= 0) {
            // Positiver Wert: Bar von Nulllinie nach oben
            barMin = ImVec2(barX, std::min(valuePixel.y, zeroY));
            barMax = ImVec2(barX + config_.barWidth, std::max(valuePixel.y, zeroY));
        } else {
            // Negativer Wert: Bar von Nulllinie nach unten
            barMin = ImVec2(barX, std::min(zeroY, valuePixel.y));
            barMax = ImVec2(barX + config_.barWidth, std::max(zeroY, valuePixel.y));
        }
        
        // Stelle sicher, dass wir ein gültiges Rechteck haben
        if (std::abs(barMax.y - barMin.y) < 1.0f) {
            // Mindesthöhe für sehr kleine Werte
            if (value >= 0) {
                barMax.y = barMin.y + 1.0f;
            } else {
                barMin.y = barMax.y - 1.0f;
            }
        }
        
        // Zeichne gefüllte Bar
        drawList->AddRectFilled(barMin, barMax, fillColor);
        
        // Zeichne Rahmen
        drawList->AddRect(barMin, barMax, borderColor, 0.0f, 0, 1.0f);
    }
}

void ImGuiBarChart::draw() {
    
    // Berechne Auto-Skalierung
    int32_t minValue, maxValue, scale;
    calculateScale(minValue, maxValue, scale);
    
    // Bereite ImGui für Zeichnung vor
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    // Reserve Platz für Labels und zentriere den Chart
    float verticalMargin = 20.0f;  // Gleicher Abstand oben und unten
    float horizontalMargin = 50.0f; // Links für Y-Achsen-Labels, rechts minimal
    
    ImVec2 chartMin = ImVec2(canvasPos.x + horizontalMargin, canvasPos.y + verticalMargin);
    ImVec2 chartMax = ImVec2(canvasPos.x + canvasSize.x - 10, canvasPos.y + canvasSize.y - verticalMargin);
    
    // ImGui Bereich reservieren
    ImGui::InvisibleButton("chart", canvasSize);
    
    // Zeichne auf ImGui DrawList
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Hintergrund
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(40, 40, 40, 100));
    drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(100, 100, 100, 255));
    
    // Zeichne Komponenten
    drawYAxis(drawList, chartMin, chartMax, minValue, maxValue, scale);
    drawXAxis(drawList, chartMin, chartMax, minValue, scale);
    drawBars(drawList, chartMin, chartMax, minValue, scale);
    
    // Titel
    std::string title = "Bewertung (Centipawn)";
    ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
    drawList->AddText(
        ImVec2(canvasPos.x + (canvasSize.x - titleSize.x) * 0.5f, canvasPos.y + 5),
        config_.textColor,
        title.c_str()
    );
}

} // namespace QaplaWindows