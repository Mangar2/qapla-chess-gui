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

#include "clock-window.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/time-control.h"

#include "imgui.h"

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

ClockWindow::ClockWindow(std::shared_ptr<BoardData> boardData)
    : boardData_(std::move(boardData))
{
}

ClockWindow::~ClockWindow() = default;


bool ClockWindow::setClockData() {
    auto& gameRecord = boardData_->gameRecord();
    auto wtc = gameRecord.getWhiteTimeControl();
    auto btc = gameRecord.getBlackTimeControl();
    if (!wtc.isValid() || !btc.isValid()) return false;
    auto [whiteTime, blackTime] = gameRecord.timeUsed();
    GoLimits goLimits = createGoLimits(wtc, btc,
        gameRecord.nextMoveIndex(), whiteTime, blackTime, gameRecord.isWhiteToMove());
    uint64_t wCur = 0;
	uint64_t bCur = 0;
    if (boardData_->engineRecords().size() >= 1) {
		wCur = boardData_->engineRecords()[0].curMoveRecord->timeMs;
    }
    if (boardData_->engineRecords().size() >= 2) {
        bCur = boardData_->engineRecords()[1].curMoveRecord->timeMs;
    }

    clockData_ = {
        .wEngineName = gameRecord.getWhiteEngineName(),
        .bEngineName = gameRecord.getBlackEngineName(),
        .wTimeLeftMs = goLimits.wtimeMs,
        .bTimeLeftMs = goLimits.btimeMs,
        .wTimeCurMove = wCur,
        .bTimeCurMove = bCur
    };
    return true;
}

void ClockWindow::draw() {
    if (!setClockData()) return;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Gesamtkoordination
    ImVec2 regionPos = ImGui::GetCursorScreenPos();
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float clockWidth = totalWidth * 0.4f;
    float spacing = totalWidth * 0.05f;
    float totalContentWidth = 2 * clockWidth + spacing;

    // Box-Größen
    float clockHeight = 60.0f; // Deine Wunschhöhe, ggf. anpassen

    // Linke Box: weiße Uhr
    ImVec2 whiteMin = ImVec2(regionPos.x + (totalWidth - totalContentWidth) * 0.5f,
        regionPos.y);
    ImVec2 whiteMax = ImVec2(whiteMin.x + clockWidth,
        whiteMin.y + clockHeight);

    // Rechte Box: schwarze Uhr
    ImVec2 blackMin = ImVec2(whiteMax.x + spacing,
        regionPos.y);
    ImVec2 blackMax = ImVec2(blackMin.x + clockWidth,
        blackMin.y + clockHeight);

    // Rahmen zeichnen (1px, kein Anti-Aliasing nötig, da rechteck)
    drawList->AddRect(whiteMin, whiteMax, IM_COL32(200, 200, 200, 255), 0.0f, 0, 1.0f);
    drawList->AddRect(blackMin, blackMax, IM_COL32(200, 200, 200, 255), 0.0f, 0, 1.0f);

    // Innen-Inhalt zentriert positionieren
    auto drawClockText = [&](ImVec2 boxMin, ImVec2 boxMax,
        const std::string& engineName,
        std::uint64_t timeLeftMs,
        std::uint64_t moveTimeMs) {
            // Mittlere X-Position
            float centerX = (boxMin.x + boxMax.x) * 0.5f;
            float y = boxMin.y + 5.0f;

            // Zeitformatieren
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%02llu:%02llu",
                timeLeftMs / 60000,
                (timeLeftMs / 1000) % 60);
            drawList->AddText(ImVec2(centerX - ImGui::CalcTextSize(buf).x * 0.5f, y),
                IM_COL32(255, 255, 255, 255), buf);

            y += ImGui::GetFontSize() + 2.0f;

            std::snprintf(buf, sizeof(buf), "Move: %llums", moveTimeMs);
            drawList->AddText(ImVec2(centerX - ImGui::CalcTextSize(buf).x * 0.5f, y),
                IM_COL32(200, 200, 200, 255), buf);

            y += ImGui::GetFontSize() + 2.0f;

            drawList->AddText(ImVec2(centerX - ImGui::CalcTextSize(engineName.c_str()).x * 0.5f, y),
                IM_COL32(200, 200, 50, 255), engineName.c_str());
        };

    drawClockText(whiteMin, whiteMax, clockData_.wEngineName, clockData_.wTimeLeftMs, clockData_.wTimeCurMove);
    drawClockText(blackMin, blackMax, clockData_.bEngineName, clockData_.bTimeLeftMs, clockData_.bTimeCurMove);

    ImGui::Dummy(ImVec2(0.0f, 0.0f));
}

