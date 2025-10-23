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
#include "os-dialogs.h"
#include "configuration.h"
#include "snackbar.h"
#include "tutorial.h"

#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-config.h"
#include "qapla-tester/engine-worker-factory.h"

#include <imgui.h>

#include <string>
#include <format>
#include <memory>
#include <filesystem>
#include <algorithm>

using namespace QaplaWindows;


EngineSetupWindow::EngineSetupWindow(bool showGlobalControls)
    : showGlobalControls_(showGlobalControls)
{
    ImGuiEngineSelect::Options options;
    options.allowGauntletEdit = true;
    options.allowNameEdit = true;
    options.allowPonderEdit = true;
    options.allowTimeControlEdit = true;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = true;
    options.allowEngineOptionsEdit = true;
    options.allowMultipleSelection = false;
    options.directEditMode = true;  
    
    engineSelect_.setOptions(options);
    engineSelect_.setId("enginesetup");
    
    // Set callback to get notified about changes
    engineSelect_.setConfigurationChangedCallback(
        [this](const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
            onEngineSelectionChanged(configurations);
        }
    );
}
EngineSetupWindow::~EngineSetupWindow() = default;

void EngineSetupWindow::onEngineSelectionChanged(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
    // Save changes back to ConfigManager
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    
    for (const auto& engineConfig : configurations) {
        // Update or add the configuration in the ConfigManager
        configManager.addOrReplaceConfig(engineConfig.config);
    }
    
    // Mark configuration as modified
    QaplaConfiguration::Configuration::instance().setModified();
}

bool EngineSetupWindow::highlighted() const {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();
    return configs.empty();
}

std::vector<QaplaTester::EngineConfig> EngineSetupWindow::getActiveEngines() const {
    std::vector<QaplaTester::EngineConfig> engines;
    
    // Extract selected engines from engineSelect_
    const auto& configurations = engineSelect_.getEngineConfigurations();
    for (const auto& config : configurations) {
        if (config.selected) {
            engines.push_back(config.config);
        }
    }
    
    // Apply global settings if enabled
    for (auto& engine : engines) {
        if (globalSettings_.useGlobalPonder) {
            engine.setPonder(globalSettings_.ponder);
        }
        if (globalSettings_.useGlobalHash) {
            engine.setOptionValue("Hash", std::to_string(globalSettings_.hashSizeMB));
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
            ImGuiEngineSelect::EngineConfiguration config;
            config.config = *matching;
            config.selected = true;
            config.originalName = matching->getName();
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
    else if (button == "Remove") {
        // Check if any engines are selected
        const auto& configurations = engineSelect_.getEngineConfigurations();
        bool hasSelection = std::ranges::any_of(configurations, 
            [](const auto& config) { return config.selected; });
        
        if (!hasSelection) {
            return QaplaButton::ButtonState::Disabled;
        }
        return QaplaButton::ButtonState::Normal;
    }
    else if (button == "Detect") {
        if (detecting) {
            return QaplaButton::ButtonState::Animated;
        }
        const auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
        if (!capabilities.areAllEnginesDetected()) {
            return QaplaButton::ButtonState::Highlighted;
        }
        return QaplaButton::ButtonState::Normal;
    }
    
    return QaplaButton::ButtonState::Normal;
}

void EngineSetupWindow::drawButtons() {
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
                }
                if (button == "Remove") {
                    QaplaButton::drawRemove(drawList, topLeft, size);
                }
                if (button == "Detect") {
                    QaplaButton::drawAutoDetect(drawList, topLeft, size, state);
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
    constexpr float controlWidth = 150.0F;
    if (!showGlobalControls_) {
        return false;
    }

    ImGui::Separator();
    ImGui::Spacing();
    
    // Global settings checkbox
    bool modified = false;
    ImGui::Indent(controlIndent_);

    if (ImGui::CollapsingHeader("Global Engine Settings")) {
        ImGui::Indent(controlIndent_);
        
        // Ponder control
        modified |= ImGui::Checkbox("##usePonder", &globalSettings_.useGlobalPonder);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(controlWidth);
        ImGui::BeginDisabled(!globalSettings_.useGlobalPonder);
        modified |= ImGui::Checkbox("Ponder", &globalSettings_.ponder);
        ImGui::EndDisabled();
       
        // Hash size control
        constexpr uint32_t maxHashMB = 64000;
        modified |= ImGui::Checkbox("##useHash", &globalSettings_.useGlobalHash);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(controlWidth);
        ImGui::BeginDisabled(!globalSettings_.useGlobalHash);
        modified |= ImGuiControls::inputInt<uint32_t>("Global Hash (MB)", globalSettings_.hashSizeMB, 1, maxHashMB);
        ImGui::EndDisabled();
        ImGui::Unindent(controlIndent_);
    }
    ImGui::Unindent(controlIndent_);
    ImGui::Spacing();
    ImGui::Separator();
    return modified;
}

void QaplaWindows::EngineSetupWindow::executeCommand(const std::string &button)
{
    try
    {
        if (button == "Add")
        {
            auto commands = OsDialogs::openFileDialog(true);
            for (auto &command : commands)
            {
                QaplaTester::EngineWorkerFactory::getConfigManagerMutable().addConfig(QaplaTester::EngineConfig::createFromPath(command));
            }
            QaplaConfiguration::Configuration::instance().setModified();
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
            QaplaConfiguration::Configuration::instance().getEngineCapabilities().autoDetect();
            QaplaConfiguration::Configuration::instance().setModified();
        }
    }
    catch (...)
    {
    }
}

void EngineSetupWindow::drawEngineList() {
    // Use ImGuiEngineSelect to draw the engine list
    engineSelect_.draw();
}

void EngineSetupWindow::draw() {
    constexpr float rightBorder = 5.0F;
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();

	drawButtons();
    drawGlobalSettings();
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("EngineList", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);
    ImGui::Indent(controlIndent_);
    drawEngineList();
    ImGui::Unindent(controlIndent_);
    ImGui::EndChild();
    
    // Check tutorial progression
    checkTutorialProgression();
}

uint32_t EngineSetupWindow::getTutorialCounter() {
    return engineSetupTutorialCounter_;
}

void EngineSetupWindow::resetTutorialCounter() {
    engineSetupTutorialCounter_ = 0;
    updateTutorialConfiguration();
}

void EngineSetupWindow::loadTutorialConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
        getConfigData().getSectionList("enginesetup", "enginesetup").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    
    if (!sections.empty()) {
        const auto& section = sections[0];
        engineSetupTutorialCounter_ = QaplaHelpers::to_uint32(section.getValue("tutorialcounter").value_or("0")).value_or(0);
    }
}

void EngineSetupWindow::updateTutorialConfiguration() {
    QaplaHelpers::IniFile::Section section {
        .name = "enginesetup",
        .entries = QaplaHelpers::IniFile::KeyValueMap{
            {"id", "enginesetup"},
            {"tutorialcounter", std::to_string(engineSetupTutorialCounter_)}
        }
    };
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("enginesetup", "enginesetup", { section });
}

void EngineSetupWindow::finishTutorial() {
    engineSetupTutorialCounter_ = 3;
    updateTutorialConfiguration();
}

void EngineSetupWindow::showNextTutorialStep() {
    engineSetupTutorialCounter_++;
    updateTutorialConfiguration();

    switch (engineSetupTutorialCounter_) {
        case 1:
        SnackbarManager::instance().showTutorial(
            "Engine Setup - Step 1\n\n"
            "To use this chess GUI, you need chess engines.\n"
            "Click the 'Add' button in the engines tab to select engine executables. "
            "You can select multiple engines at once in the file dialog.\n\n"
            "Please add 2 or more engines to continue.",
            SnackbarManager::SnackbarType::Note, false);
        return;
        case 2:
        SnackbarManager::instance().showTutorial(
            "Engine Setup - Step 2\n\n"
            "Great! You have added engines.\n"
            "Now click the 'Detect' button to automatically read all options from your engines.\n"
            "It runs in parallel for all engines. Still it may take a few seconds.",
            SnackbarManager::SnackbarType::Note, false);
        return;
        case 3:
        SnackbarManager::instance().showTutorial(
            "Engine Setup Complete!\n\n"
            "Excellent! Your engines are now configured and ready to use.\n"
            "You can select them in other tabs like Tournament or Engine Test.\n\n"
            "Engine setup tutorial completed!",
            SnackbarManager::SnackbarType::Success, false);
        return;
        default:
        return;
    }
}

void EngineSetupWindow::checkTutorialProgression() {
    // Only start tutorial if Snackbar tutorial is completed
    if (!Tutorial::instance().isCompleted(Tutorial::Topic::Snackbar)) {
        return;
    }

    switch (engineSetupTutorialCounter_) {
        case 0:
        showNextTutorialStep();
        return;
        case 1:
        {
            const auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
            const auto configs = configManager.getAllConfigs();
            if (configs.size() >= 2) {
                showNextTutorialStep();
            }
        }
        return;
        case 2:
        {
            const auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
            if (capabilities.areAllEnginesDetected()) {
                showNextTutorialStep();
            }
        }
        return;
        default:
        return;
    }
}

