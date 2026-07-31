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

    ImGui::PushID(label);

    // Input field
    ImGui::SetNextItemWidth(width);
    ImGuiControls::inputText(label, currentValue_);
    // Commit as soon as the field loses focus after an edit (Enter, Tab, clicking
    // elsewhere) -- not only on an explicit Apply click, so a typed change can never be
    // silently lost just because the separate Apply button wasn't clicked.
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sourceValue = currentValue_;
        originalValue_ = currentValue_;
        applied = true;
    }
    ImGuiControls::hooverTooltip("Edit value and press Enter/click elsewhere, or click Apply, to save changes");

    // Apply button (for committing without leaving the field, e.g. before adjusting another setting)
    ImGui::SameLine();
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

    ImGui::PopID();

    return applied;
}

ConfigurationWindow::ConfigurationWindow() = default;
ConfigurationWindow::~ConfigurationWindow() = default;

namespace {
    constexpr std::size_t MAX_LLM_HOST_HISTORY = 8;

    // Moves `host` to the front of `history` (adding it if new), capped so the dropdown
    // doesn't grow without bound across a long-running install.
    void rememberLlmHost(std::vector<std::string>& history, const std::string& host) {
        std::erase(history, host);
        history.insert(history.begin(), host);
        if (history.size() > MAX_LLM_HOST_HISTORY) {
            history.resize(MAX_LLM_HOST_HISTORY);
        }
    }
}

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

    if (ImGuiControls::CollapsingHeaderWithDot("LM Studio / AI Chat Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0F);
        drawLlmChatConfig();
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

void ConfigurationWindow::drawLlmChatConfig()
{
    constexpr float inputWidth = 200.0F;

    auto config = QaplaConfiguration::Configuration::getLlmChatConfig();
    bool modified = false;

    bool enabled = config.enabled;
    if (ImGui::Checkbox("Offer AI Chat when LM Studio is detected", &enabled)) {
        config.enabled = enabled;
        modified = true;
    }
    ImGuiControls::hooverTooltip(
        "If enabled, the app automatically checks for LM Studio at startup and adds an "
        "\"AI Chat\" entry to the chatbot menu when found.");

    ImGui::Spacing();
    ImGui::Text("LM Studio Server:");

    bool hostChanged = false;

    if (!config.hostHistory.empty()) {
        std::string selectedHost = config.host;
        ImGui::SetNextItemWidth(inputWidth);
        if (ImGuiControls::selectionBox("Recent Hosts", selectedHost, config.hostHistory)) {
            config.host = selectedHost;
            modified = true;
            hostChanged = true;
        }
        ImGuiControls::hooverTooltip(
            "Switch back to a previously used LM Studio server address without retyping it.");
    }

    if (llmChatHostInput_.draw("Host / Address", config.host, inputWidth)) {
        modified = true;
        hostChanged = true;
    }
    ImGuiControls::hooverTooltip(
        "Hostname or IP address of the machine running LM Studio -- \"localhost\" for this "
        "machine, or e.g. \"192.168.1.42\" / \"my-server.local\" for a remote one. For a "
        "remote server, enable \"Serve on Local Network\" on LM Studio's Developer tab so "
        "it accepts connections from other machines.");

    ImGui::SetNextItemWidth(inputWidth);
    if (ImGuiControls::inputInt<int>("Port", config.port, 1, 65535)) {
        modified = true;
    }
    ImGuiControls::hooverTooltip("LM Studio's server port, shown on its Developer tab (default 1234).");

    ImGui::Spacing();
    bool logTraffic = config.logTraffic;
    if (ImGui::Checkbox("Log AI chat traffic to file", &logTraffic)) {
        config.logTraffic = logTraffic;
        modified = true;
    }
    ImGuiControls::hooverTooltip(
        "Appends every AI chat conversation (what you said, what the model answered, and any "
        "tool/connection errors) to a timestamped log file in the app's config directory -- "
        "useful for diagnosing an unhelpful or misbehaving model after the fact.");

    if (modified) {
        if (hostChanged && !config.host.empty()) {
            rememberLlmHost(config.hostHistory, config.host);
        }
        QaplaConfiguration::Configuration::setLlmChatConfig(config);
    }
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
