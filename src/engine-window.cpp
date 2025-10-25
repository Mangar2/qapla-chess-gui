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
#include "imgui-engine-list.h"
#include "imgui-button.h"
#include "snackbar.h"
#include "tutorial.h"

#include "qapla-tester/engine-worker-factory.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>
#include <set>

using namespace QaplaWindows;

EngineWindow::EngineWindow() = default;
EngineWindow::~EngineWindow() = default;

std::string EngineWindow::drawConfigButtonArea(bool noEnginesSelected, bool enginesAvailable) {
    constexpr float borderX = 20.0F;
    constexpr float borderY = 8.0F;
    constexpr float spacingY = 30.0F;

    constexpr ImVec2 buttonSize = { 25.0F, 25.0F };
    auto topLeft = ImGui::GetCursorScreenPos();

    std::string command;

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + borderX, topLeft.y + borderY));
    auto state = QaplaButton::ButtonState::Normal;
    if (noEnginesSelected && enginesAvailable) {
        state = QaplaButton::ButtonState::Highlighted;
    }
    if (QaplaButton::drawIconButton("Config", "Config", buttonSize, state,
        [state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            QaplaButton::drawConfig(drawList, topLeft, size, state);
        }))
    {
        command = "Config";
    }

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + borderX, topLeft.y + borderY + buttonSize.y + spacingY));
    if (QaplaButton::drawIconButton("SwapButton", "Swap", buttonSize, 
        noEnginesSelected ? QaplaButton::ButtonState::Disabled : QaplaButton::ButtonState::Normal,
        [state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            QaplaButton::drawSwapEngines(drawList, topLeft, size, state);
        }))
    {
        command = "Swap";
    }

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + areaWidth, topLeft.y));
    ImGuiSeparator::Vertical();
    return command;
}

std::pair<std::string, std::string> EngineWindow::draw() {
    constexpr float cMinRowHeight = 80.0F;
    constexpr float cEngineInfoWidth = 160.0F;
    constexpr float cMinTableWidth = 200.0F;
    constexpr float cSectionSpacing = 4.0F;
    constexpr float rightBorder = 5.0F;

    const auto engineRecords = getEngineRecords();
    const auto engineAvailable = !QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs().empty();
    auto command = drawConfigButtonArea(engineRecords.empty(), engineAvailable);
    ImGui::Indent(areaWidth);
    auto [id2, command2] = ImGuiEngineList::draw();
    ImGui::Unindent(areaWidth);
    
    // Create a list of active engines from the engine records
    std::vector<QaplaTester::EngineConfig> activeEngines;
    for (const auto& record : engineRecords) {
        activeEngines.push_back(record.config);
    }
    
    // Check tutorial progression
    bool configCommandIssued = (command == "Config");
    showNextTutorialStep(configCommandIssued, activeEngines);
    
    if (!command.empty()) { 
        return { "", command };
    }
    return { id2, command2 };
}

static auto engineWindowTutorialInit = []() {
    Tutorial::instance().addEntry({
        .name = "enginewindow",
        .displayName = "Engine Window",
        .dependsOn = "enginesetup",
        .messages = {
            { "Engine Window - Step 1\n\n"
              "Welcome to the Engine Window!\n"
              "Here you can select which engines to use for analysis or play.\n\n"
              "Click the Config button (gear icon) on the left to open the engine selection popup.",
              SnackbarManager::SnackbarType::Note },
            { "Engine Window - Step 2\n\n"
              "Great! You've opened the engine selection.\n"
              "You can select multiple engines, and the same engine can be selected multiple times.\n\n"
              "Now please select two different engines to continue.",
              SnackbarManager::SnackbarType::Note },
            { "Engine Window Complete!\n\n"
              "Excellent! You've successfully selected engines for playing.\n"
              "Next we will use the engines.",
              SnackbarManager::SnackbarType::Success }
        },
        .getProgressCounter = []() -> uint32_t& {
            return EngineWindow::tutorialProgress_;
        },
        .autoStart = false
    });
    return true;
}();

void EngineWindow::showNextTutorialStep(bool configCommandIssued, const std::vector<QaplaTester::EngineConfig>& activeEngines) {
    switch (tutorialProgress_) {
        case 0:
        Tutorial::instance().showNextTutorialStep("enginewindow");
        return;
        case 1:
        if (configCommandIssued) {
            Tutorial::instance().showNextTutorialStep("enginewindow");
        }
        return;
        case 2:
        {
            std::set<std::string> uniqueEngines;
            for (const auto& engine : activeEngines) {
                uniqueEngines.insert(engine.getCmd());
            }
            if (uniqueEngines.size() >= 2) {
                Tutorial::instance().showNextTutorialStep("enginewindow");
            }
        }
        return;
        default:
        Tutorial::instance().finishTutorial("enginewindow");
        return;
    }
}

