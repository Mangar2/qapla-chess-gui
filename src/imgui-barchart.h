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

#pragma once

#include "qapla-tester/game-record.h"

#include <imgui.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>

namespace QaplaWindows {

    struct BarChartConfig {
        // Y-Achse Skalierung (in Centipawn)
        int32_t minScaleCP = 200;     // Minimum Skalierung +/- 200cp
        int32_t maxScaleCP = 1000;   // Maximum Skalierung +/- 10000cp
        
        // Chart Dimensionen
        float barWidth = 8.0f;
        float barSpacing = 2.0f;
        
        // Farben für abwechselnde Balken (Weiß/Schwarz Züge)
        ImU32 whiteBarFill = IM_COL32(220, 220, 255, 180);    // Helle Füllung für Weiß
        ImU32 whiteBarBorder = IM_COL32(100, 100, 150, 255);  // Rahmen für Weiß
        ImU32 blackBarFill = IM_COL32(180, 180, 200, 180);    // Dunklere Füllung für Schwarz
        ImU32 blackBarBorder = IM_COL32(60, 60, 80, 255);     // Rahmen für Schwarz
        
        // Achsenfarben
        ImU32 axisColor = IM_COL32(150, 150, 150, 255);
        ImU32 textColor = IM_COL32(200, 200, 200, 255);
        ImU32 gridColor = IM_COL32(100, 100, 100, 100);
    };
    
    class ImGuiBarChart {
    public:
        ImGuiBarChart() : config_() {}
        
        ImGuiBarChart(const BarChartConfig& config) : config_(config) {}

        void addValue(int32_t value) {
            values_.push_back(value);
        }
        
        void clearValues() {
            values_.clear();
        }
        
        void setConfig(const BarChartConfig& config) {
            config_ = config;
        }
        
        const BarChartConfig& getConfig() const {
            return config_;
        }

        void setFromGameRecord(const GameRecord& gameRecord) {
            clearValues();
            for (const auto& move : gameRecord.history()) {
                addValue(move.scoreCp.value_or(0));
            }
        }

        void draw();

    private:
        std::vector<int32_t> values_;
        BarChartConfig config_;
        
        // Private Hilfsmethoden
        void calculateScale(int32_t& minValue, int32_t& maxValue, int32_t& scale) const;
        void drawXAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const;
        void drawYAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t maxValue, int32_t scale) const;
        void drawBars(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const;
        ImVec2 valueToPixel(float moveIndex, int32_t value, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const;
    };

} // namespace QaplaWindows