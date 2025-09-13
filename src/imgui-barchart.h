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
#include "qapla-tester/change-tracker.h"

#include <imgui.h>
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>

namespace QaplaWindows {

    struct BarChartConfig {
        // Y-axis scaling (in centipawn)
        int32_t minScaleCP = 200;     // Minimum scaling +/- 200cp
        int32_t maxScaleCP = 1000;   // Maximum scaling +/- 10000cp
        
        // Chart dimensions
        float barWidth = 10.0f;
        float minBarWidth = 2.0f;
        float barSpacing = 0.0f;

        float verticalMargin = 10.0f;
        float leftMargin = 30.0f;
        float rightMargin = 20.0f;
        
        // Colors for alternating bars (white/black moves)
        ImU32 whiteBarFill = IM_COL32(220, 220, 255, 180);    // Light fill for white
        ImU32 whiteBarBorder = IM_COL32(100, 100, 150, 255);  // Border for white
        
        ImU32 blackBarFill = IM_COL32(140, 140, 160, 180);    // Darker fill for black
        ImU32 blackBarBorder = IM_COL32(60, 60, 80, 255);     // Border for black
        
        // Axis colors
        ImU32 axisColor = IM_COL32(150, 150, 150, 255);
        ImU32 textColor = IM_COL32(200, 200, 200, 255);
        ImU32 gridColor = IM_COL32(100, 100, 100, 100);

        // Title of the chart
        std::string title = "Evaluation";
    };
    
    class ImGuiBarChart {
    public:
        ImGuiBarChart() : config_() {}
        
        ImGuiBarChart(const BarChartConfig& config) : config_(config) {}

        /**
         * Adds a single centipawn value to the bar chart data.
         * @param value The centipawn value to add (positive or negative).
         */
        void addValue(int32_t value) {
            values_.push_back(value);
        }
        
        /**
         * Clears all data values from the bar chart.
         */
        void clearValues() {
            values_.clear();
        }
        
        /**
         * Sets the configuration for the bar chart.
         * @param config The BarChartConfig struct containing desired settings.
         */
        void setConfig(const BarChartConfig& config) {
            config_ = config;
        }
        
        /**
         * Returns the current configuration of the bar chart.
         * @return The BarChartConfig struct containing current settings.
         */
        const BarChartConfig& getConfig() const {
            return config_;
        }

        /**
         * Initializes the bar chart data from a GameRecord's move history.
         * Extracts centipawn scores from each move and populates the internal values vector.
         * @param gameRecord The GameRecord containing the move history to extract scores from.
         */
        void setFromGameRecord(const GameRecord& gameRecord);

        /**
         * Main drawing function that renders the complete bar chart
         * Handles dynamic bar width calculation, component rendering order, and layout
         */
        void draw();

    private:
        std::vector<int32_t> values_;
        BarChartConfig config_;
        ChangeTracker changeTracker_;
        
        /**
         * Calculates the Y-axis scale based on data values and configuration limits
         * @param minValue Output: minimum scale value (negative)
         * @param maxValue Output: maximum scale value (positive)  
         * @param scale Output: total scale range (maxValue - minValue)
         */
        void calculateScale(int32_t& minValue, int32_t& maxValue, int32_t& scale) const;

        /**
         * Draws the X-axis with move number labels using adaptive spacing
         * @param drawList ImGui draw list for rendering
         * @param chartMin Top-left corner of chart area
         * @param chartMax Bottom-right corner of chart area
         * @param minValue Minimum scale value for zero line calculation
         * @param scale Total scale range for zero line calculation
         */
        void drawXAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const;

        /**
         * Draws the Y-axis with centipawn value labels and grid lines
         * Adapts label density based on available space and uses symmetric scaling
         * @param drawList ImGui draw list for rendering
         * @param chartMin Top-left corner of chart area
         * @param chartMax Bottom-right corner of chart area
         * @param minValue Minimum scale value
         * @param maxValue Maximum scale value
         * @param scale Total scale range
         */
        void drawYAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t maxValue, int32_t scale) const;

        /**
         * Draws individual bars representing centipawn values for each move
         * Uses alternating colors for white/black moves and ensures proper rectangle rendering
         * @param drawList ImGui draw list for rendering
         * @param chartMin Top-left corner of chart area
         * @param chartMax Bottom-right corner of chart area
         * @param minValue Minimum scale value for positioning
         * @param scale Total scale range for positioning
         */
        void drawBars(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const;

        /**
         * Converts a data point to pixel coordinates within the chart area
         * @param moveIndex Index of the move (0-based)
         * @param value Centipawn value to convert
         * @param chartMin Top-left corner of chart area
         * @param chartMax Bottom-right corner of chart area
         * @param minValue Minimum scale value
         * @param scale Total scale range
         * @return Pixel coordinates for the data point
         */
        ImVec2 valueToPixel(float moveIndex, int32_t value, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const;
    };

} // namespace QaplaWindows