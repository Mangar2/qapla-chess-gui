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

#include "imgui-engine-global-settings.h"
#include "imgui-engine-controls.h"
#include "imgui-controls.h"
#include "configuration.h"
#include "i18n.h"
#include "tournament-config-sections.h"
#include "tutorial.h"

#include <engine-handling/engine-config.h>
#include <engine-handling/engine-option.h>

#include <imgui.h>

using namespace QaplaWindows;

namespace {

    constexpr const char* restartLabels[] = { "Engine decides", "Always", "Never" };

    /**
     * @brief Names a restart mode the way the control lists it.
     *
     * A mode read from a tournament or SPRT file comes in the file format's spelling
     * ("auto"/"on"/"off"), which matches none of the labels - the control would fall back to
     * its first entry and show "Engine decides" for an engine that is set to never restart.
     * @param value The stored value, in either spelling.
     * @return The matching label, or the value unchanged if it names no known mode.
     */
    [[nodiscard]] std::string toRestartLabel(const std::string& value) {
        try {
            switch (QaplaTester::parseRestartOption(value)) {
            case QaplaTester::RestartOption::Always: return restartLabels[1];
            case QaplaTester::RestartOption::Never: return restartLabels[2];
            default: return restartLabels[0];
            }
        }
        catch (const std::exception&) {
            return value;
        }
    }

} // namespace

ImGuiEngineGlobalSettings::ImGuiEngineGlobalSettings(ConfigurationChangedCallback callback)
    : configurationCallback_(std::move(callback))
    , timeControlCallback_(nullptr)
{
}

bool ImGuiEngineGlobalSettings::drawGlobalSettings(DrawControlOptions controls, 
    const Options& options,
    const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    ImGuiTreeNodeFlags flags = options.alwaysOpen ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None;
    
    if (ImGuiControls::CollapsingHeaderWithDot("Global Engine Settings", flags, tutorialContext.highlight)) {
        ImGui::Indent(controls.controlIndent);
        
        if (options.showHash) {
            modified |= drawHashControl(controls.controlWidth, options.showUseCheckboxes, tutorialContext);
        }
        if (options.showRestart) {
            modified |= drawRestartControl(controls.controlWidth, options.showUseCheckboxes, tutorialContext);
        }
        if (options.showTrace) {
            modified |= drawTraceControl(controls.controlWidth, options.showUseCheckboxes, tutorialContext);
        }
        if (options.showPonder) {
            modified |= drawPonderControl(controls.controlWidth, options.showUseCheckboxes, tutorialContext);
        }
        if (options.showSyzygy) {
            modified |= drawSyzygyControls(controls.controlWidth, tutorialContext);
        }
        
        ImGui::Unindent(controls.controlIndent);
    }
    
    if (modified) {
        notifyConfigurationChanged();
    }
    
    return modified;
}

bool ImGuiEngineGlobalSettings::drawHashControl(float controlWidth, bool showUseCheckboxes, 
    const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    constexpr uint32_t maxHashMB = 64000;
    
    if (showUseCheckboxes) {
        modified |= ImGui::Checkbox("##useHash", &globalSettings_.useGlobalHash);
        ImGuiControls::hooverTooltip("Enable global hash size setting for all engines");
        ImGui::SameLine();
    } else {
        globalSettings_.useGlobalHash = true;
    }
    ImGui::SetNextItemWidth(controlWidth);
    ImGui::BeginDisabled(!globalSettings_.useGlobalHash);
    modified |= ImGuiControls::inputInt<uint32_t>("Hash (MB)", globalSettings_.hashSizeMB, 1, maxHashMB);
    ImGuiControls::hooverTooltip("Hash table size in megabytes for engine memory");
    ImGui::EndDisabled();
    
    auto it = tutorialContext.annotations.find("Hash (MB)");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    return modified;
}

bool ImGuiEngineGlobalSettings::drawRestartControl(float controlWidth, bool showUseCheckboxes,
    const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    
    if (showUseCheckboxes) {
        modified |= ImGui::Checkbox("##useRestart", &globalSettings_.useGlobalRestart);
        ImGuiControls::hooverTooltip("Enable global restart policy for all engines");
        ImGui::SameLine();
    } else {
        globalSettings_.useGlobalRestart = true;
    }
    ImGui::SetNextItemWidth(controlWidth);
    ImGui::BeginDisabled(!globalSettings_.useGlobalRestart);
    globalSettings_.restart = toRestartLabel(globalSettings_.restart);
    modified |= ImGuiControls::selectionBox("Restart", globalSettings_.restart,
        {restartLabels[0], restartLabels[1], restartLabels[2]});
    ImGuiControls::hooverTooltip("Whether to restart engine process between games");
    ImGui::EndDisabled();
    
    auto it = tutorialContext.annotations.find("Restart");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    return modified;
}

bool ImGuiEngineGlobalSettings::drawTraceControl(float controlWidth, bool showUseCheckboxes,
    const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    
    if (showUseCheckboxes) {
        modified |= ImGui::Checkbox("##useTrace", &globalSettings_.useGlobalTrace);
        ImGuiControls::hooverTooltip("Enable global trace level for all engines");
        ImGui::SameLine();
    } else {
        globalSettings_.useGlobalTrace = true;
    }
    ImGui::SetNextItemWidth(controlWidth);
    ImGui::BeginDisabled(!globalSettings_.useGlobalTrace);
    modified |= ImGuiControls::selectionBox("Trace", globalSettings_.traceLevel,
        {"None", "All", "Command"});
    ImGuiControls::hooverTooltip("Engine communication logging level (None/All/Command only)");
    ImGui::EndDisabled();
    
    auto it = tutorialContext.annotations.find("Trace");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    return modified;
}

bool ImGuiEngineGlobalSettings::drawPonderControl(float controlWidth, bool showUseCheckboxes,
    const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    
    if (showUseCheckboxes) {
        modified |= ImGui::Checkbox("##usePonder", &globalSettings_.useGlobalPonder);
        ImGuiControls::hooverTooltip("Enable global pondering setting for all engines");
        ImGui::SameLine();
    } else {
        globalSettings_.useGlobalPonder = true;
    }
    ImGui::SetNextItemWidth(controlWidth);
    ImGui::BeginDisabled(!globalSettings_.useGlobalPonder);
    modified |= ImGui::Checkbox("Ponder", &globalSettings_.ponder);
    ImGuiControls::hooverTooltip("Allow engines to think during opponent's time");
    ImGui::EndDisabled();
    
    auto it = tutorialContext.annotations.find("Ponder");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    return modified;
}

bool ImGuiEngineGlobalSettings::drawSyzygyControls(float controlWidth,
    const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    constexpr uint32_t maxProbeDepth = 100;
    constexpr uint32_t maxProbeLimit = 7;

    // The switch is always drawn -- see the declaration for why it does not follow
    // showUseCheckboxes like the settings above it.
    modified |= ImGuiControls::checkbox("Syzygy tablebases", globalSettings_.useGlobalSyzygy);
    ImGuiControls::hooverTooltip(
        "Configure the tablebase settings for all engines at once.\n"
        "Switched off, every engine keeps the tablebase settings of its own.");

    ImGui::BeginDisabled(!globalSettings_.useGlobalSyzygy);
    ImGui::Indent(10.0F);

    // Translated here rather than by the control: existingDirectoryInput() shows its label as
    // given, because the engine option editor uses it with option names, which are never
    // translated.
    modified |= ImGuiControls::existingDirectoryInput(
        Translator::instance().translate("Input", "Tablebase folder"),
        globalSettings_.syzygyPath, controlWidth);
    ImGuiControls::hooverTooltip(
        "The folder holding the .rtbw and .rtbz files (UCI option 'SyzygyPath').\n"
        "Several folders are separated the way the engine expects it.");

    ImGui::SetNextItemWidth(controlWidth);
    modified |= ImGuiControls::inputInt<uint32_t>("Probe Depth", globalSettings_.syzygyProbeDepth,
        1, maxProbeDepth);
    ImGuiControls::hooverTooltip(
        "Least search depth at which the tablebases are read (UCI option 'SyzygyProbeDepth').\n"
        "Higher values probe less often and cost less time.");

    ImGui::SetNextItemWidth(controlWidth);
    modified |= ImGuiControls::inputInt<uint32_t>("Probe Limit", globalSettings_.syzygyProbeLimit,
        0, maxProbeLimit);
    ImGuiControls::hooverTooltip(
        "Largest tablebase to read, counted in pieces (UCI option 'SyzygyProbeLimit').\n"
        "Set it to the largest set of tables you actually have; 0 switches probing off.");

    modified |= ImGuiControls::checkbox("50 Move Rule", globalSettings_.syzygy50MoveRule);
    ImGuiControls::hooverTooltip(
        "Whether a tablebase result counts the fifty-move rule (UCI option 'Syzygy50MoveRule').\n"
        "Switched off, wins that need more than fifty moves are reported as wins.");

    ImGui::Unindent(10.0F);
    ImGui::EndDisabled();

    auto it = tutorialContext.annotations.find("Syzygy tablebases");
    if (it != tutorialContext.annotations.end()) {
        ImGuiControls::annotate(it->second);
    }
    return modified;
}

bool ImGuiEngineGlobalSettings::drawTimeControl(DrawControlOptions controls, 
    bool blitz, bool alwaysOpen, const Tutorial::TutorialContext& tutorialContext) {
    bool modified = false;
    ImGuiTreeNodeFlags flags = alwaysOpen ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None;
    
    if (ImGuiControls::CollapsingHeaderWithDot("Time Control", flags, tutorialContext.highlight)) {
        ImGui::Indent(controls.controlIndent);
        
        // Both controls work with the SAME variable (timeControlSettings_.timeControl)
        // This ensures automatic synchronization: selecting "50.0+0.10" in the dropdown
        // automatically updates the input field, and vice versa - no separate sync needed
        modified |= ImGuiControls::timeControlInput(timeControlSettings_.timeControl, blitz, 
            controls.controlWidth);
        ImGuiControls::hooverTooltip("Time control format: seconds+increment (e.g., '60.0+0.5' for 60s + 0.5s/move)");
        
        ImGui::SetNextItemWidth(controls.controlWidth);
        modified |= ImGuiControls::selectionBox("Predefined time control", timeControlSettings_.timeControl, 
                                        timeControlSettings_.predefinedOptions);
        ImGuiControls::hooverTooltip(
            "Quick selection for common time controls.\n"
            "Selecting an option automatically fills the input fields above.\n"
            "Example: '20.0+0.02' sets Seconds=20, Increment Ms=20"
        );
        
        // Show tutorial annotation if present
        auto it = tutorialContext.annotations.find("Predefined time control");
        if (it != tutorialContext.annotations.end()) {
            ImGuiControls::annotate(it->second);
        }
        
        ImGui::Unindent(controls.controlIndent);
    }
    
    if (modified) {
        notifyTimeControlChanged();
    }
    
    return modified;
}

void ImGuiEngineGlobalSettings::setGlobalConfiguration(const QaplaTester::EngineGlobalConfig& globalSettings) {
    globalSettings_ = globalSettings;
    notifyConfigurationChanged();
}

void ImGuiEngineGlobalSettings::setTimeControlSettings(const TimeControlSettings& timeControlSettings) {
    timeControlSettings_ = timeControlSettings;
    notifyTimeControlChanged();
}

void ImGuiEngineGlobalSettings::setConfigurationChangedCallback(ConfigurationChangedCallback callback) {
    configurationCallback_ = std::move(callback);
}

void ImGuiEngineGlobalSettings::setTimeControlChangedCallback(TimeControlChangedCallback callback) {
    timeControlCallback_ = std::move(callback);
}

void ImGuiEngineGlobalSettings::notifyConfigurationChanged() {
    updateConfiguration();
    if (configurationCallback_) {
        configurationCallback_(globalSettings_);
    }
}

void ImGuiEngineGlobalSettings::notifyTimeControlChanged() {
    updateTimeControlConfiguration();
    if (timeControlCallback_) {
        timeControlCallback_(timeControlSettings_);
    }
}

void ImGuiEngineGlobalSettings::updateConfiguration() const {
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "each", id_, {{
            .name = "each",
            .entries = QaplaHelpers::IniFile::KeyValueMap{
                {"id", id_},
                {"usehash", globalSettings_.useGlobalHash ? "true" : "false"},
                {"hash", std::to_string(globalSettings_.hashSizeMB)},
                {"useponder", globalSettings_.useGlobalPonder ? "true" : "false"},
                {"ponder", globalSettings_.ponder ? "true" : "false"},
                {"usetrace", globalSettings_.useGlobalTrace ? "true" : "false"},
                {"trace", globalSettings_.traceLevel},
                {"userestart", globalSettings_.useGlobalRestart ? "true" : "false"},
                {"restart", globalSettings_.restart},
                // Written whether the setting is switched on or not, like the settings above:
                // this is the GUI's own file, where a switched-off value has to survive a restart.
                {"usesyzygy", globalSettings_.useGlobalSyzygy ? "true" : "false"},
                {"syzygypath", globalSettings_.syzygyPath},
                {"syzygyprobedepth", std::to_string(globalSettings_.syzygyProbeDepth)},
                {"syzygyprobelimit", std::to_string(globalSettings_.syzygyProbeLimit)},
                {"syzygy50moverule", globalSettings_.syzygy50MoveRule ? "true" : "false"}
            }
    }});
}

void ImGuiEngineGlobalSettings::updateTimeControlConfiguration() const {
    // Build predefined options entries
    QaplaHelpers::IniFile::KeyValueMap entries{
        {"id", id_},
        {"timeControl", timeControlSettings_.timeControl}
    };
    
    // Add predefined options
    for (size_t i = 0; i < timeControlSettings_.predefinedOptions.size(); ++i) {
        entries.emplace_back("predefinedOption" + std::to_string(i), timeControlSettings_.predefinedOptions[i]);
    }
    
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "timecontroloptions", id_, {{
            .name = "timecontroloptions",
            .entries = std::move(entries)
    }});
}

void ImGuiEngineGlobalSettings::setGlobalConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
    for (const auto& section : sections) {
        if (section.name == "each") {
            auto newGlobalSettings = QaplaConfiguration::fromEachSection(section);
            // A state file has no timecontroloptions section: it carries the time control as
            // the "each" key "tc", the way the CLI expects it. A section naming none -- the
            // GUI's own, which keeps it in timecontroloptions -- leaves the setting alone.
            const bool namesTimeControl = !newGlobalSettings.timeControl.empty();
            if (namesTimeControl) {
                timeControlSettings_.timeControl = newGlobalSettings.timeControl;
            } else {
                newGlobalSettings.timeControl = timeControlSettings_.timeControl;
            }

            globalSettings_ = newGlobalSettings;
            notifyConfigurationChanged();
            if (namesTimeControl) {
                notifyTimeControlChanged();
            }
            break;
        }
    }
}

void ImGuiEngineGlobalSettings::setTimeControlConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
    for (const auto& section : sections) {
        if (section.name == "timecontroloptions") {
            TimeControlSettings newTimeControlSettings;
            
            // Load time control
            if (auto value = section.getValue("timeControl")) {
                newTimeControlSettings.timeControl = *value;
            }
            
            // Load predefined options
            newTimeControlSettings.predefinedOptions.clear();
            for (size_t i = 0; ; ++i) {
                auto value = section.getValue("predefinedOption" + std::to_string(i));
                if (!value) {
                    break;
                }
                newTimeControlSettings.predefinedOptions.push_back(*value);
            }
            
            // If no predefined options were loaded, use defaults
            if (newTimeControlSettings.predefinedOptions.empty()) {
                newTimeControlSettings.predefinedOptions = timeControlSettings_.predefinedOptions;
            }
            
            timeControlSettings_ = newTimeControlSettings;
            notifyTimeControlChanged();
            break;
        }
    }
}
