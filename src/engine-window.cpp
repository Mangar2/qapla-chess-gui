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

#include "engine-window.h"
#include "imgui-separator.h"
#include "imgui-popup.h"
#include "imgui-engine-list.h"
#include "imgui-button.h"
#include "engine-setup-window.h"

#include "imgui.h"

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

EngineWindow::EngineWindow()
    : setupWindow_(std::make_unique<ImGuiPopup<EngineSetupWindow>>(
        ImGuiPopup<EngineSetupWindow>::Config{ .title = "Select Engines" }))
{
}

EngineWindow::~EngineWindow() = default;

void EngineWindow::drawEngineSelectionPopup() {
	setupWindow_->draw("Use", "Cancel");
    auto& boardData = QaplaWindows::BoardData::instance();
    if (auto confirmed = setupWindow_->confirmed()) {
        if (*confirmed) {
			boardData.setEngines(setupWindow_->content().getActiveEngines());
        }
        setupWindow_->resetConfirmation();
    }
}

void EngineWindow::drawConfigButtonArea() {
    constexpr float areaWidth = 80.0f;
    constexpr auto button = "Config";
    constexpr ImVec2 buttonSize = { 25.0f, 25.0f };
    auto topLeft = ImGui::GetCursorScreenPos();

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + 20.0f, topLeft.y + 8.0f));
    if (QaplaButton::drawIconButton(button, button, buttonSize, false,
        [&button](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            QaplaButton::drawConfig(drawList, topLeft, size);
        }))
    {
        auto& boardData = QaplaWindows::BoardData::instance();
        std::vector<EngineConfig> activeEngines;
        for (const auto& record : boardData.engineRecords()) {
            activeEngines.push_back(record.config);
        }
        setupWindow_->content().setMatchingActiveEngines(activeEngines);
        setupWindow_->open();
    }
    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + 65.0f, topLeft.y));
    ImGuiSeparator::Vertical();
}

void EngineWindow::draw() {
    constexpr float cMinRowHeight = 80.0f;
    constexpr float cEngineInfoWidth = 160.0f;
    constexpr float cMinTableWidth = 200.0f;
    constexpr float cSectionSpacing = 4.0f;

    ImVec2 cursorBefore = ImGui::GetCursorScreenPos();
    drawConfigButtonArea();
    ImVec2 topleft = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float indent = topleft.x - cursorBefore.x;
    ImGui::Indent(indent);

    auto& boardData = QaplaWindows::BoardData::instance();
    const auto engineRecords = boardData.engineRecords();
	const auto records = engineRecords.empty() ? 1 : engineRecords.size();

    const float tableMinWidth = std::max(cMinTableWidth, avail.x - cEngineInfoWidth - cSectionSpacing);
    const uint32_t rowHeight = static_cast<uint32_t>(
        std::max(cMinRowHeight, avail.y / static_cast<float>(records)));

    auto [index, command] = boardData.imGuiEngineList().draw();
    ImGui::Unindent(indent);
    try {
        if (command == "Restart") {
            boardData.restartEngine(index);
        }
        else if (command == "Stop") {
            boardData.stopEngine(index);
        }
        else if (command == "Config") {
            std::vector<EngineConfig> activeEngines;
            for (const auto& record : boardData.engineRecords()) {
                activeEngines.push_back(record.config);
            }
            setupWindow_->content().setMatchingActiveEngines(activeEngines);
            setupWindow_->open();
        }
    }
    catch (...) {

    }
    drawEngineSelectionPopup();
}

