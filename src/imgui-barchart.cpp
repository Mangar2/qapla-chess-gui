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

#include <qapla-engine/evalvalue.h>

#include <ranges>
#include <sstream>
#include <iomanip>

using QaplaTester::GameRecord;

namespace QaplaWindows {

namespace {

/**
 * @brief Calculates the appropriate step size for Y-axis labels based on ideal step size.
 */
int32_t calculateYStepSize(int32_t idealStepSize) {
    if (idealStepSize <= 100) {
        return 100;
    }
    if (idealStepSize <= 250) {
        return 200;
    }
    if (idealStepSize <= 500) {
        return 500;
    }
    if (idealStepSize <= 1000) {
        return 1000;
    }
    if (idealStepSize <= 2500) {
        return 2500;
    }
    if (idealStepSize <= 5000) {
        return 5000;
    }
    return 10000;
}

/**
 * @brief Adjusts step size when window is too small for standard spacing.
 */
int32_t adjustStepSizeForSmallWindow(int32_t stepSize, float chartHeight, int32_t scale) {
    float availableSpacingForTwo = chartHeight / 2.0F;
    auto maxValueThatFits = static_cast<int32_t>((availableSpacingForTwo / chartHeight) * scale);
    
    int32_t adjustedStepSize = stepSize;
    while (adjustedStepSize > maxValueThatFits && adjustedStepSize > 25) {
        if (adjustedStepSize >= 1000) {
            adjustedStepSize /= 2;
        } else if (adjustedStepSize >= 500) {
            adjustedStepSize = 200;
        } else {
            adjustedStepSize = 100;
        }
    }
    return adjustedStepSize;
}

/**
 * @brief Draws a horizontal grid line with tick mark and label on the Y-axis.
 */
void drawYAxisGridLine(
    ImDrawList* drawList,
    float y,
    float chartMinX,
    float chartMaxX,
    int32_t value,
    ImU32 gridColor,
    ImU32 axisColor,
    ImU32 textColor
) {
    // Grid line
    drawList->AddLine(
        ImVec2(chartMinX, y),
        ImVec2(chartMaxX, y),
        gridColor,
        1.0F
    );
    
    // Tick mark
    drawList->AddLine(
        ImVec2(chartMinX - 3, y),
        ImVec2(chartMinX + 3, y),
        axisColor,
        1.0F
    );
    
    // Label
    std::string label = std::to_string(value / 100);
    ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    drawList->AddText(
        ImVec2(chartMinX - textSize.x - 8, y - textSize.y * 0.5F),
        textColor,
        label.c_str()
    );
}

} // anonymous namespace

void ImGuiBarChart::setFromGameRecord(const GameRecord& gameRecord) {
    auto [isModified, isUpdated] = changeTracker_.checkModification(gameRecord.getChangeTracker());
    if (!isUpdated) {
        return;
    }
    changeTracker_.updateFrom(gameRecord.getChangeTracker());
    if (isModified) {
        clearValues();
    }

    for (uint32_t index = values_.size(); index < gameRecord.history().size(); ++index) {
        const auto& move = gameRecord.history()[index];
        int32_t score = move.scoreCp.value_or(0);
        if (move.scoreMate.has_value()) {
            score = (move.scoreMate.value() > 0) ? QaplaBasics::MAX_VALUE : -QaplaBasics::MAX_VALUE;
        }
        if (!gameRecord.wtmAtPly(index)) {
            score = -score;
        }
        addValue(score);
    }
}


void ImGuiBarChart::calculateScale(int32_t& minValue, int32_t& maxValue, int32_t& scale) const {
    if (values_.empty()) {
        minValue = -config_.minScaleCP;
        maxValue = config_.minScaleCP;
        scale = config_.minScaleCP * 2;
        return;
    }
    
    auto [minIt, maxIt] = std::ranges::minmax_element(values_);
    int32_t dataMin = *minIt;
    int32_t dataMax = *maxIt;
    
    int32_t maxAbsValue = std::max(std::abs(dataMin), std::abs(dataMax));
    
    if (maxAbsValue < config_.minScaleCP) {
        maxAbsValue = config_.minScaleCP;
    } else if (maxAbsValue > config_.maxScaleCP) {
        maxAbsValue = config_.maxScaleCP;
    }
    
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
    
    float yNormalized = static_cast<float>(value - minValue) / static_cast<float>(scale);
    float y = chartMax.y - yNormalized * (chartMax.y - chartMin.y);
    
    return {x, y};
}

void ImGuiBarChart::drawXAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const {
    float zeroY = chartMax.y - (static_cast<float>(0 - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
    
    drawList->AddLine(
        ImVec2(chartMin.x - 5, zeroY),
        ImVec2(chartMax.x + 10, zeroY),
        config_.axisColor,
        2.0F
    );
    
    if (values_.empty()) {
        return;
    }
    
    float availableWidth = chartMax.x - chartMin.x;
    float avgLabelWidth = ImGui::CalcTextSize("100").x;
    float minLabelSpacing = avgLabelWidth * 1.5F;
    
    int maxLabels = static_cast<int>(availableWidth / minLabelSpacing);
    maxLabels = std::max(2, std::min(maxLabels, static_cast<int>(values_.size())));
    
    int totalValues = static_cast<int>(values_.size());
    int idealStep = std::max(1, totalValues / (maxLabels - 1));
    
    int stepSize;
    if (idealStep <= 10) {
        stepSize = 10;
    } else if (idealStep <= 20) {
        stepSize = 20;
    } else if (idealStep <= 40) {
        stepSize = 40;
    } else if (idealStep <= 100) {
        stepSize = 100;
    } else {
        stepSize = 200;
    }
    
    for (int step = 1; step * stepSize <= totalValues; ++step) {
        int moveNumber = step * stepSize;
        int arrayIndex = moveNumber - 1;
        
        float x = chartMin.x + arrayIndex * (config_.barWidth + config_.barSpacing) + config_.barWidth * 0.5F;
        
        drawList->AddLine(
            ImVec2(x, zeroY - 3),
            ImVec2(x, zeroY + 3),
            config_.axisColor,
            1.0F
        );
        
        std::string label = std::to_string(moveNumber / 2);
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        drawList->AddText(
            ImVec2(x - textSize.x * 0.5F, zeroY + 8),
            config_.textColor,
            label.c_str()
        );
    }
}


void ImGuiBarChart::drawYAxis(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t maxValue, int32_t scale) const {
    drawList->AddLine(
        ImVec2(chartMin.x, chartMin.y - 10),
        ImVec2(chartMin.x, chartMax.y + 10),
        config_.axisColor,
        2.0F
    );
    
    float chartHeight = chartMax.y - chartMin.y;
    float textHeight = ImGui::GetTextLineHeight();
    float minLabelSpacing = textHeight * 1.5F;
    
    int maxLabels = static_cast<int>(chartHeight / minLabelSpacing);
    maxLabels = std::max(3, std::min(maxLabels, 15));
    
    int32_t idealStepSize = scale / (maxLabels - 1);
    int32_t stepSize = calculateYStepSize(idealStepSize);
    
    bool showZero = maxLabels > 3;
    
    if (!showZero) {
        stepSize = adjustStepSizeForSmallWindow(stepSize, chartHeight, scale);
    }
    
    if (showZero) {
        int32_t value = 0;
        float y = chartMax.y - (static_cast<float>(value - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
        
        drawList->AddLine(
            ImVec2(chartMin.x - 3, y),
            ImVec2(chartMin.x + 3, y),
            config_.axisColor,
            1.0F
        );
        
        std::string label = "0";
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        drawList->AddText(
            ImVec2(chartMin.x - textSize.x - 8, y - textSize.y * 0.5F),
            config_.textColor,
            label.c_str()
        );
    }
    
    int32_t maxSteps = showZero ? (maxLabels - 1) / 2 : maxLabels / 2;
    
    if (!showZero && maxSteps == 0) {
        maxSteps = 1;
    }
    
    for (int32_t step = 1; step <= maxSteps; ++step) {
        int32_t positiveValue = step * stepSize;
        int32_t negativeValue = -step * stepSize;
        
        if (positiveValue <= maxValue || (!showZero && step == 1)) {
            float y = chartMax.y - (static_cast<float>(positiveValue - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
            drawYAxisGridLine(drawList, y, chartMin.x, chartMax.x, positiveValue, config_.gridColor, config_.axisColor, config_.textColor);
        }
        
        if (negativeValue >= minValue || (!showZero && step == 1)) {
            float y = chartMax.y - (static_cast<float>(negativeValue - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
            drawYAxisGridLine(drawList, y, chartMin.x, chartMax.x, negativeValue, config_.gridColor, config_.axisColor, config_.textColor);
        }
    }
}

void ImGuiBarChart::drawBars(ImDrawList* drawList, const ImVec2& chartMin, const ImVec2& chartMax, int32_t minValue, int32_t scale) const {
    float zeroY = chartMax.y - (static_cast<float>(0 - minValue) / static_cast<float>(scale)) * (chartMax.y - chartMin.y);
    
    for (size_t i = 0; i < values_.size(); ++i) {
        int32_t value = values_[i];
        
        bool isWhiteMove = (i % 2 == 0);
        ImU32 fillColor = isWhiteMove ? config_.whiteBarFill : config_.blackBarFill;
        ImU32 borderColor = isWhiteMove ? config_.whiteBarBorder : config_.blackBarBorder;
        
        float barX = chartMin.x + i * (config_.barWidth + config_.barSpacing);
        ImVec2 valuePixel = valueToPixel(static_cast<float>(i), value, chartMin, chartMax, minValue, scale);
        
        ImVec2 barMin;
        ImVec2 barMax;
        if (value >= 0) {
            barMin = ImVec2(barX, std::min(valuePixel.y, zeroY));
            barMax = ImVec2(barX + config_.barWidth, std::max(valuePixel.y, zeroY));
        } else {
            barMin = ImVec2(barX, std::min(zeroY, valuePixel.y));
            barMax = ImVec2(barX + config_.barWidth, std::max(zeroY, valuePixel.y));
        }
        
        // Ensure minimum bar height for visibility
        if (std::abs(barMax.y - barMin.y) < 1.0F) {
            if (value >= 0) {
                barMax.y = barMin.y + 1.0F;
            } else {
                barMin.y = barMax.y - 1.0F;
            }
        }
        
        drawList->AddRectFilled(barMin, barMax, fillColor);
        drawList->AddRect(barMin, barMax, borderColor, 0.0F, 0, 1.0F);
    }
}

std::optional<uint32_t> ImGuiBarChart::draw() {
    
    int32_t minValue;
    int32_t maxValue;
    int32_t scale;
    calculateScale(minValue, maxValue, scale);
    
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    auto chartMin = ImVec2(canvasPos.x + config_.leftMargin, canvasPos.y + config_.verticalMargin);
    auto chartMax = ImVec2(canvasPos.x + canvasSize.x - config_.rightMargin, 
        canvasPos.y + canvasSize.y - config_.verticalMargin);

    float availableWidth = chartMax.x - chartMin.x;
    auto totalBarsNeeded = static_cast<float>(values_.size());
    float dynamicBarWidth = config_.barWidth;
    
    if (totalBarsNeeded > 0) {
        float spacePerBar = availableWidth / totalBarsNeeded;
        float calculatedBarWidth = spacePerBar - config_.barSpacing;
        dynamicBarWidth = std::max(config_.minBarWidth, std::min(calculatedBarWidth, config_.barWidth));
    }
    
    BarChartConfig tempConfig = config_;
    tempConfig.barWidth = dynamicBarWidth;
    
    BarChartConfig originalConfig = config_;
    const_cast<ImGuiBarChart*>(this)->config_ = tempConfig;
    
    bool clicked = ImGui::InvisibleButton("chart", canvasSize);
    std::optional<uint32_t> clickedHalfMove = std::nullopt;
    
    // Check for click detection
    if (clicked && !values_.empty()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        // Check if click is within chart area
        if (mousePos.x >= chartMin.x && mousePos.x <= chartMax.x &&
            mousePos.y >= chartMin.y && mousePos.y <= chartMax.y) {
            
            // Calculate which bar was clicked based on x position
            float relativeX = mousePos.x - chartMin.x;
            auto barIndex = static_cast<uint32_t>(relativeX / (dynamicBarWidth + config_.barSpacing));
            
            // Ensure the bar index is valid and the click is actually on a bar
            if (barIndex < values_.size()) {
                float barX = barIndex * (dynamicBarWidth + config_.barSpacing);
                if (relativeX >= barX && relativeX <= barX + dynamicBarWidth) {
                    // Half-move number is bar index + 1 (since chess moves are 1-indexed)
                    clickedHalfMove = barIndex + 1;
                }
            }
        }
    }
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    drawYAxis(drawList, chartMin, chartMax, minValue, maxValue, scale);
    drawBars(drawList, chartMin, chartMax, minValue, scale);
    drawXAxis(drawList, chartMin, chartMax, minValue, scale);
    
    const_cast<ImGuiBarChart*>(this)->config_ = originalConfig;
    
    std::string title = config_.title;
    ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
    drawList->AddText(
        ImVec2(canvasPos.x + (canvasSize.x - titleSize.x) * 0.5F, canvasPos.y + 5),
        config_.textColor,
        title.c_str()
    );
    
    return clickedHalfMove;
}

} // namespace QaplaWindows