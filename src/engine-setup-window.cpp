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

#include "engine-setup-window.h"
#include "imgui-table.h"
#include "imgui-button.h"
#include "imgui-controls.h"
#include "imgui-engine-controls.h"
#include "imgui-engine-select.h"
#include "imgui-engine-global-settings.h"
#include "os-dialogs.h"
#include "configuration.h"
#include "snackbar.h"
#include "tutorial.h"

#include "string-helper.h"
#include "engine-config.h"
#include "engine-worker-factory.h"

#include <imgui.h>

#include <string>
#include <format>
#include <memory>
#include <filesystem>
#include <algorithm>

namespace QaplaWindows {

EngineSetupWindow::EngineSetupWindow(bool showGlobalControls)
    : showGlobalControls_(showGlobalControls)
{
    ImGuiEngineSelect::Options options;
    // Hack, the use-case showing globalcontrols assumes protocol is not editable and vice versa
    options.allowProtocolEdit = !showGlobalControls;
    options.allowGauntletEdit = false;
    options.allowNameEdit = true;
    options.allowPonderEdit = true;
    options.allowTimeControlEdit = true;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = true;
    options.allowEngineOptionsEdit = true;
    options.allowMultipleSelection = false;
    options.directEditMode = true;  
    options.enginesDefaultOpen = true;
    
    engineSelect_.setOptions(options);
    engineSelect_.setId("");
    
    ImGuiEngineGlobalSettings::Options globalOptions;
    globalOptions.showHash = true;
    globalOptions.showPonder = true;
    globalOptions.showTrace = true;
    globalOptions.showRestart = true;
    
    globalSettings_.setOptions(globalOptions);
    globalSettings_.setId("");
}

EngineSetupWindow::~EngineSetupWindow() = default;

bool EngineSetupWindow::highlighted() const {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();
    return configs.empty();
}

std::vector<QaplaTester::EngineConfig> EngineSetupWindow::getActiveEngines() const {
    std::vector<QaplaTester::EngineConfig> engines;
    
    const auto& configurations = engineSelect_.getEngineConfigurations();
    for (const auto& config : configurations) {
        if (config.selected) {
            auto engine = config.config;
            ImGuiEngineGlobalSettings::applyGlobalConfig(engine, globalSettings_.getGlobalConfiguration(), 
                                                         globalSettings_.getTimeControlSettings());
            engines.push_back(engine);
        }
    }
    
    return engines;
}

void EngineSetupWindow::setMatchingActiveEngines(const std::vector<QaplaTester::EngineConfig>& engines) {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    
    std::vector<ImGuiEngineSelect::EngineConfiguration> configurations;
    
    for (const auto& engine : engines) {
        auto* matching = configManager.getConfigMutableByCmdAndProtocol(engine.getCmd(), engine.getProtocol());
        if (matching != nullptr) {
            ImGuiEngineSelect::EngineConfiguration config {
                .config = engine,
                .selected = true,
                .originalName = engine.getName()
            };
            configurations.push_back(config);
        }
    }
    
    engineSelect_.setEngineConfigurations(configurations);
}

QaplaButton::ButtonState EngineSetupWindow::getButtonState(const std::string& button) const {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();
    bool detecting = QaplaConfiguration::Configuration::instance().getEngineCapabilities().isDetecting();
    
    if (button == "Add") {
        if (configs.empty()) {
            return QaplaButton::ButtonState::Highlighted;
        }
        return QaplaButton::ButtonState::Normal;
    }
    if (button == "Remove") {
        // Check if any engines are selected
        const auto& configurations = engineSelect_.getEngineConfigurations();
        bool hasSelection = std::ranges::any_of(configurations, 
            [](const auto& config) { return config.selected; });
        
        if (!hasSelection) {
            return QaplaButton::ButtonState::Disabled;
        }
        return QaplaButton::ButtonState::Normal;
    }
    if (button == "Detect") {
        if (detecting) {
            return QaplaButton::ButtonState::Animated;
        }
        if (!ImGuiEngineSelect::areAllEnginesDetected()) {
            return QaplaButton::ButtonState::Highlighted;
        }
        return QaplaButton::ButtonState::Normal;
    }
    
    return QaplaButton::ButtonState::Normal;
}

void EngineSetupWindow::drawButtons() {
    if (!showButtons_) {
        return;
    }
    constexpr float space = 3.0F;
    constexpr float topOffset = 5.0F;
    constexpr float bottomOffset = 8.0F;
    constexpr float leftOffset = 20.0F;
    
    ImVec2 topLeft = ImGui::GetCursorScreenPos();
    topLeft.x = std::round(topLeft.x);
    topLeft.y = std::round(topLeft.y);
    auto curPos = ImVec2(topLeft.x + leftOffset, topLeft.y + topOffset);
    
    std::vector<std::string> buttons{ "Add", "Remove", "Detect" };
    constexpr ImVec2 buttonSize = { 25.0F, 25.0F };
    auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, buttons);

    for (const auto& button : buttons) {
        auto state = getButtonState(button);
        
        ImGui::SetCursorScreenPos(curPos);
        if (QaplaButton::drawIconButton(button, button, buttonSize, state,
            [&button, state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
                if (button == "Add") {
                    QaplaButton::drawAdd(drawList, topLeft, size);
                    ImGuiControls::hooverTooltip("Add new engine from executable file");
                }
                if (button == "Remove") {
                    QaplaButton::drawRemove(drawList, topLeft, size);
                    ImGuiControls::hooverTooltip("Remove selected engine from configuration");
                }
                if (button == "Detect") {
                    QaplaButton::drawAutoDetect(drawList, topLeft, size, state);
                    ImGuiControls::hooverTooltip("Auto-detect engine capabilities and supported options");
                }
            }))
        {
            executeCommand(button);
        }
        curPos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x, topLeft.y + totalSize.y + topOffset + bottomOffset));
}

bool EngineSetupWindow::drawGlobalSettings() {
    if (!showGlobalControls_) {
        return false;
    }
    return globalSettings_.drawGlobalSettings(
        { .controlWidth = 150.0F, .controlIndent = controlIndent_ });
}

void QaplaWindows::EngineSetupWindow::executeCommand(const std::string &button)
{

    if (button == "Add")
    {
        engineSelect_.addEngines(false);
    }
    else if (button == "Remove")
    {
        // Remove selected engines
        const auto& configurations = engineSelect_.getEngineConfigurations();
        for (const auto& config : configurations) {
            if (config.selected) {
                QaplaTester::EngineWorkerFactory::getConfigManagerMutable().removeConfig(config.config);
                QaplaConfiguration::Configuration::instance().getEngineCapabilities().deleteCapability(
                    config.config.getCmd(), config.config.getProtocol());
            }
        }
        
        // Clear selection after removal
        std::vector<ImGuiEngineSelect::EngineConfiguration> emptyConfigs;
        engineSelect_.setEngineConfigurations(emptyConfigs);
    }
    else if (button == "Detect")
    {
        try {
            QaplaConfiguration::Configuration::instance().getEngineCapabilities().autoDetect();
        } catch (const std::exception& ex) {
            SnackbarManager::instance().showWarning(
                std::format("Engine auto-detect failed,\nsome engines may not be detected\n {}", ex.what()));
        }
        QaplaConfiguration::Configuration::instance().setModified();
    }

}

void EngineSetupWindow::draw() {
    constexpr float rightBorder = 5.0F;
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();

	drawButtons();
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("EngineList", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);
    ImGui::Indent(controlIndent_);
    ImGui::Spacing();
    drawGlobalSettings();
    ImGui::Spacing();
    ImGui::Separator();
    engineSelect_.draw();
    ImGui::Unindent(controlIndent_);
    ImGui::EndChild();
    
    // Check tutorial progression
    showNextTutorialStep();
}

static auto engineSetupTutorialInit = []() {
    Tutorial::instance().setEntry({
        .name = Tutorial::TutorialName::EngineSetup,
        .displayName = "Engine Setup",
        .messages = {
            { .text = "To use this chess GUI, you need chess engines.\n"
              "Click the 'Add' button in the engines tab to select engine executables. "
              "You can select multiple engines at once in the file dialog.\n\n"
              "Please add 2 or more engines to continue.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Great! You have added engines.\n"
              "Now click the 'Detect' button to automatically read all options from your engines.\n"
              "It runs in parallel for all engines. Still it may take a few seconds.",
              .type = SnackbarManager::SnackbarType::Note },
            { .text = "Engine Setup Complete!\n\n"
              "Excellent! Your engines are now configured and ready to use.\n"
              "You can select them in other tabs like Tournament or Engine Test.\n\n"
              "Engine setup tutorial completed!",
              .type = SnackbarManager::SnackbarType::Success }
        },
        .getProgressCounter = []() -> uint32_t& {
            return EngineSetupWindow::tutorialProgress_;
        },
        .autoStart = true
    });
    return true;
}();

void EngineSetupWindow::showNextTutorialStep() {
    constexpr auto tutorialName = Tutorial::TutorialName::EngineSetup;
    switch (tutorialProgress_) {
        case 0:
        Tutorial::instance().requestNextTutorialStep(tutorialName);
        return;
        case 1:
        {
            const auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
            const auto configs = configManager.getAllConfigs();
            if (configs.size() >= 2) {
                Tutorial::instance().requestNextTutorialStep(tutorialName);
            }
        }
        return;
        case 2:
        {
            const auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
            if (capabilities.areAllEnginesDetected()) {
                Tutorial::instance().requestNextTutorialStep(tutorialName);
            }
        }
        return;
        case 3:
        if (!SnackbarManager::instance().isTutorialMessageVisible()) {
            Tutorial::instance().finishTutorial(tutorialName);
        }
        return;
        default:
        // No other case defined, intentionally left blank
        return;
    }
}

} // namespace QaplaWindows
