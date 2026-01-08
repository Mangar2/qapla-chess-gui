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

#include "imgui-controls.h"
#include "base-elements/string-helper.h"
#include "engine-handling/engine-config.h"
#include "engine-handling/engine-option.h"
#include "base-elements/logger.h"
#include "configuration.h"

#include <imgui.h>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

namespace QaplaWindows::ImGuiEngineControls {

/**
 * @brief Converts string to TraceLevel
 */
inline QaplaTester::TraceLevel stringToTrace(const std::string& str) {
    const auto lowerStr = QaplaHelpers::to_lowercase(str);
    if (lowerStr == "none") return QaplaTester::TraceLevel::none;
    if (lowerStr == "all") return QaplaTester::TraceLevel::info;
    return QaplaTester::TraceLevel::command;
}

/**
 * @brief Draws an engine name input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineName(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string name = config.getName();
    if (ImGuiControls::inputText("Name", name)) {
        config.setName(name);
        return true;
    }
    ImGuiControls::hooverTooltip("Display name for the engine");
    return false;
}

/**
 * @brief Draws an engine author input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineAuthor(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string author = config.getAuthor();
    if (ImGuiControls::inputText("Author", author)) {
        config.setAuthor(author);
        return true;
    }
    ImGuiControls::hooverTooltip("Engine author name");
    return false;
}

/**
 * @brief Draws an engine command input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineCommand(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string cmd = config.getCmd();
    if (ImGuiControls::inputText("Command", cmd)) {
        config.setCmd(cmd);
        return true;
    }
    ImGuiControls::hooverTooltip("Executable path or command to launch the engine");
    return false;
}

/**
 * @brief Draws an engine directory input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineDirectory(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string dir = config.getDir();
    if (ImGuiControls::existingDirectoryInput("Directory", dir)) {
        config.setDir(dir);
        return true;
    }
    ImGuiControls::hooverTooltip("Working directory for the engine process");
    return false;
}

/**
 * @brief Draws an engine command-line arguments input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineArguments(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string args = config.getArgs();
    if (ImGuiControls::inputText("Arguments", args)) {
        config.setArgs(args);
        return true;
    }
    ImGuiControls::hooverTooltip("Command-line arguments to pass to the engine executable");
    return false;
}

/**
 * @brief Draws a protocol selection combo box
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineProtocol(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::vector<std::string> labels = {"UCI", "XBoard"};
    
    std::string protocolStr = (config.getProtocol() == QaplaTester::EngineProtocol::Uci) ? "UCI" : "XBoard";

    if (ImGuiControls::selectionBox("Protocol", protocolStr, labels)) {
        config.setProtocol(QaplaTester::parseEngineProtocol(protocolStr));
        return true;
    }
    ImGuiControls::hooverTooltip("Chess engine communication protocol (UCI or XBoard)");
    return false;
}

/**
 * @brief Draws a trace level selection combo box
 * @param traceLevel Trace level to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineTraceLevel(QaplaTester::TraceLevel& traceLevel, bool enabled) {
    if (!enabled) return false;
    
    std::vector<std::string> labels = { "None", "All", "Command" };
    
    auto traceStr = to_string(traceLevel);

    if (ImGuiControls::selectionBox("Trace", traceStr, labels)) {
        traceLevel = stringToTrace(traceStr);
        return true;
    }
    ImGuiControls::hooverTooltip("Engine communication logging level (None/All/Command only)");
    return false;
}

/**
 * @brief Draws a trace level selection combo box
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineTraceLevel(QaplaTester::EngineConfig& config, bool enabled) {
    return drawEngineTraceLevel(config.getTraceLevel(), enabled);
}

/**
 * @brief Draws a restart option selection combo box
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineRestartOption(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::vector<std::string> labels = {"Engine decides", "Always", "Never"};
    
    std::string restartStr;
    switch (config.getRestartOption()) {
        case QaplaTester::RestartOption::EngineDecides: restartStr = "Engine decides"; break;
        case QaplaTester::RestartOption::Always: restartStr = "Always"; break;
        case QaplaTester::RestartOption::Never: restartStr = "Never"; break;
        default: restartStr = "Engine decides"; break;
    }

    if (ImGuiControls::selectionBox("Restart", restartStr, labels)) {
        config.setRestartOption(QaplaTester::parseRestartOption(restartStr));
        return true;
    }
    ImGuiControls::hooverTooltip("Whether to restart engine process between games");
    return false;
}

/**
 * @brief Draws a ponder checkbox control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEnginePonder(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    bool ponder = config.isPonderEnabled();
    if (ImGuiControls::checkbox("Ponder", ponder)) {
        config.setPonder(ponder);
        return true;
    }
    ImGuiControls::hooverTooltip("Allow engine to think during opponent's time");
    return false;
}

/**
 * @brief Draws a gauntlet checkbox control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineGauntlet(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    bool result = ImGuiControls::checkbox("Gauntlet", config.gauntlet());
    ImGuiControls::hooverTooltip("Mark this engine to play against all others in gauntlet mode");
    return result;
}

inline bool drawEngineScoreFromWhitePov(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    bool result = ImGuiControls::checkbox("Score from White POV", config.scoreFromWhitePov());
    ImGuiControls::hooverTooltip("Engine reports scores from white's perspective regardless of side to move");
    return result;
}

/**
 * @brief Draws a time control input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineTimeControl(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string tcString = config.getTimeControl().toPgnTimeControlString();
    if (ImGuiControls::inputText("Time Control", tcString)) {
        try {
            config.setTimeControl(tcString);
            return true;
        } catch (const std::exception&) {
            // Ignore invalid input
        }
    }
    ImGuiControls::hooverTooltip("Time control format: seconds+increment (e.g., '60.0+0.5' for 60s + 0.5s/move)");
    return false;
}

/**
 * @brief Draws engine-specific options controls using capabilities
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if any option was changed
 */
inline bool drawEngineOptions(QaplaTester::EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
    auto& capability = capabilities.getCapability(config.getCmd(), config.getProtocol());
    if (!capability) return false;
    
    auto& options = capability->getSupportedOptions();
    if (options.empty()) return false;
    
    bool changed = false;
    ImGui::Separator();
    ImGui::Text("Engine Options:");
    ImGui::PushID("EngineOptions");
    auto optionMap = config.getOptionValues();
    for (const auto& option : options) {
        auto it = optionMap.find(option.name);
        std::string value = (it != optionMap.end()) ? it->second : option.defaultValue;
        if (ImGuiControls::engineOptionControl(option, value, 400.0F)) {
            changed = true;
            config.setOptionValue(option.name, value);
        }
    }
    ImGui::PopID();
    
    return changed;
}

/**
 * @brief Draws read-only engine information
 * @param config Engine configuration to display
 * @param full Whether to show full details (directory and author)
 */
inline void drawEngineReadOnlyInfo(const QaplaTester::EngineConfig& config, bool full = false, bool protocol = true) {
    ImGui::Text(full ? "Command: %s" : "%s", config.getCmd().c_str());
    if (full) {
        ImGui::Text("Directory: %s", config.getDir().c_str());
    }
    if (protocol) {
        ImGui::Text("Protocol: %s", 
            (config.getProtocol() == QaplaTester::EngineProtocol::Uci ? "UCI" : "XBoard"));
    }
    if (full) {
        ImGui::Text("Author: %s", config.getAuthor().c_str());
    }
}

} // namespace QaplaWindows::ImGuiEngineControls