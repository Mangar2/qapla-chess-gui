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

#include "configuration-window.h"
#include "engine-setup-window.h"
#include "imgui-controls.h"
#include "snackbar.h"
#include "configuration.h"
#include "tutorial.h"
#include "os-dialogs.h"
#include "i18n.h"

#include <base-elements/logger.h>

#include <imgui.h>
#include <map>
#include <vector>

using namespace QaplaWindows;

bool BufferedTextInput::draw(const char* label, std::string& sourceValue, float width) {
    // Check if source value changed externally (e.g., loaded from config)
    if (originalValue_ != sourceValue) {
        originalValue_ = sourceValue;
        currentValue_ = sourceValue;
    }
    
    bool applied = false;
    
    // Apply button
    ImGui::PushID(label);
    const bool hasChanges = (currentValue_ != originalValue_);
    if (!hasChanges) {
        ImGui::BeginDisabled();
    }
    if (ImGuiControls::textButton("Apply")) {
        sourceValue = currentValue_;
        originalValue_ = currentValue_;
        applied = true;
    }
    ImGuiControls::hooverTooltip("Apply pending changes to configuration");
    if (!hasChanges) {
        ImGui::EndDisabled();
    }
    
    // Input field
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGuiControls::inputText(label, currentValue_);
    ImGuiControls::hooverTooltip("Edit value and click Apply to save changes");
    
    ImGui::PopID();
    
    return applied;
}

ConfigurationWindow::ConfigurationWindow() = default;
ConfigurationWindow::~ConfigurationWindow() = default;

static void drawLanguageConfig() {
    std::string currentLanguageCode = Translator::instance().getLanguageCode();
    
    std::map<std::string, std::string, std::less<>> langMap = {
        {"English", "eng"},
        {"Deutsch", "deu"},
        {"Français", "fra"}
    };    
    
    std::string currentLanguageName = "English"; 
    std::vector<std::string> languageNames;
    
    for (const auto& [key, value] : langMap) {
        if (value == currentLanguageCode) {
            currentLanguageName = key;
        }
        languageNames.push_back(key);
    }
    if (ImGuiControls::selectionBox("Language", currentLanguageName, languageNames)) {
        auto& newLanguageCode = langMap[currentLanguageName];
        QaplaConfiguration::Configuration::updateLanguageConfiguration(newLanguageCode);
        Translator::instance().setLanguageCode(newLanguageCode);
    }
    ImGuiControls::hooverTooltip(tr("Tooltip", "Translation is currently work in progress. Only few parts are translated."));
}

void ConfigurationWindow::draw()
{
    ImGui::Spacing();
    
    if (ImGuiControls::CollapsingHeaderWithDot("Snackbar Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawSnackbarConfig();
        ImGui::Unindent(10.0F);
    }
    
    ImGui::Spacing();
    /*
    // Disabled for now, tutorials are started in the chatbot now
    if (ImGuiControls::CollapsingHeaderWithDot("Tutorial Progress", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawTutorialConfig();
        ImGui::Unindent(10.0F);
    }
    
    ImGui::Spacing();
    */
    if (ImGuiControls::CollapsingHeaderWithDot("Logger Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawLoggerConfig();
        ImGui::Unindent(10.0F);
    }
    
    ImGui::Spacing();

    if (ImGuiControls::CollapsingHeaderWithDot("Language Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawLanguageConfig();
        ImGui::Unindent(10.0F);
    }
    
    ImGui::Spacing();
    
    if (ImGuiControls::CollapsingHeaderWithDot("Performance Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawPerformanceConfig();
        ImGui::Unindent(10.0F);
    }
    
    ImGui::Spacing();
}

void ConfigurationWindow::drawSnackbarConfig()
{
    constexpr float inputWidth = 200.0F;
    constexpr uint32_t minDuration = 1;
    constexpr uint32_t maxDuration = 120;
    bool modified = false;

    auto& config = SnackbarManager::instance().getConfig();

    ImGui::Text("Message Display Durations (in seconds):");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Note Duration", config.noteDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }
    ImGuiControls::hooverTooltip("Display duration for informational messages in seconds");

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Success Duration", config.successDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }
    ImGuiControls::hooverTooltip("Display duration for success messages in seconds");

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Warning Duration", config.warningDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }
    ImGuiControls::hooverTooltip("Display duration for warning messages in seconds");

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<uint32_t>("Error Duration", config.errorDurationInS, minDuration, maxDuration))
    {
        modified = true;
    }
    ImGuiControls::hooverTooltip("Display duration for error messages in seconds");

    if (modified) {
        SnackbarManager::instance().updateConfiguration();
    }
}

void ConfigurationWindow::drawTutorialConfig()
{
    ImGui::Text("Tutorial Topics:");
    ImGui::Spacing();

    // Loop in TutorialName enum Reihenfolge (garantiert sortiert)
    for (size_t i = 0; i < static_cast<size_t>(Tutorial::TutorialName::Count); ++i) {
        auto tutorialName = static_cast<Tutorial::TutorialName>(i);
        auto& entry = Tutorial::instance().getEntry(tutorialName);
        
        bool completed = !entry.running();
        if (ImGui::Checkbox(entry.displayName.c_str(), &completed)) {
            if (completed) {
                Tutorial::instance().finishTutorial(tutorialName);
            } else {
                Tutorial::instance().restartTutorial(tutorialName);
            }
        }
        ImGuiControls::hooverTooltip(completed ? "Tutorial completed - uncheck to restart" : "Tutorial not completed - check to mark as complete");
        ImGui::SameLine();
        auto messageCount = entry.messages.size();
        auto progress = std::min<uint32_t>(entry.getProgressCounter(), messageCount);
        ImGui::TextDisabled("(Progress: %u/%zu)", progress, messageCount);
    }
    
    ImGui::Spacing();
    if (ImGuiControls::textButton("Reset All Tutorials")) {
        Tutorial::instance().resetAll();
    }
    ImGuiControls::hooverTooltip("Resets all tutorials to their initial state");
}

void ConfigurationWindow::drawPerformanceConfig()
{
    ImGui::Spacing();

    bool remoteDesktopMode = QaplaConfiguration::Configuration::isRemoteDesktopMode();
    if (ImGui::Checkbox("Remote Desktop Mode", &remoteDesktopMode)) {
        QaplaConfiguration::Configuration::setRemoteDesktopMode(remoteDesktopMode);
        SnackbarManager::instance().showNote(
            "Please restart the application for this change to take effect",
            false, "performance"
        );
    }
    ImGuiControls::hooverTooltip(
        "Optimizes performance for Remote Desktop / RDP / X11 forwarding\n"
        "- Reduces frame rate from 60 to 30 FPS\n"
        "- Disables VSync for better responsiveness\n"
        "- Removes decorative background image\n"
        "Requires restart to apply changes."
    );
}

void ConfigurationWindow::drawLoggerConfig()
{
    constexpr float inputWidth = 200.0F;
    
    auto config = QaplaTester::getLoggerConfig();
    bool modified = false;

    // Log Path - only via dialog, no manual typing
    ImGui::Text("Log Directory:");
    if (ImGuiControls::textButton("Browse###LogPath")) {
        try {
            auto selectedPath = OsDialogs::selectFolderDialog(config.logPath);
            if (!selectedPath.empty()) {
                config.logPath = selectedPath;
                modified = true;
            }
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showError(e.what());
        }
    }
    ImGuiControls::hooverTooltip("Select directory for log files");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", config.logPath.c_str());
    ImGui::Spacing();

    if (reportBaseNameInput_.draw("Report Log - Base Name", config.reportLogBaseName, inputWidth)) {
        modified = true;
    }

    if (engineBaseNameInput_.draw("Engine Log - Base Name", config.engineLogBaseName, inputWidth)) {
        modified = true;
    }

    ImGui::SetNextItemWidth(inputWidth);
    int currentStrategy = static_cast<int>(config.engineLogStrategy);
    const std::vector<std::string> strategyItems = { 
        "Global (single file for all engines)", "Per Engine (one file per engine instance)" };
    if (ImGuiControls::selectionBox("Engine Log File Strategy", currentStrategy, strategyItems)) {
        config.engineLogStrategy = static_cast<QaplaTester::LogFileStrategy>(currentStrategy);
        modified = true;
    }
    const char* tooltip = "";
    switch (currentStrategy) {
        case 0: tooltip = "All engine instance communication is logged to a single file"; break;
        case 1: tooltip = "Each engine gets its own log file"; break;
        case 2: tooltip = "All engines in a game share a single log file"; break;
    }
    ImGuiControls::hooverTooltip(tooltip);

    if (modified) {
        QaplaTester::setLoggerConfig(config);
        QaplaConfiguration::Configuration::updateLoggerConfiguration();
    }
}
