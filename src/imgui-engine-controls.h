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
#include "qapla-tester/engine-config.h"
#include "qapla-tester/engine-option.h"
#include "qapla-tester/logger.h"
#include "configuration.h"

#include <imgui.h>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

namespace QaplaWindows::ImGuiEngineControls {

/**
 * @brief Converts TraceLevel to string representation
 */
inline std::string traceToString(TraceLevel level) {
    switch (level) {
        case TraceLevel::none: return "none";
        case TraceLevel::command: return "command";
        case TraceLevel::info: return "all";
        default: return "command";
    }
}

/**
 * @brief Converts string to TraceLevel
 */
inline TraceLevel stringToTrace(const std::string& str) {
    if (str == "none") return TraceLevel::none;
    if (str == "all") return TraceLevel::info;
    return TraceLevel::command;
}

/**
 * @brief Draws an engine name input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineName(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string name = config.getName();
    if (ImGuiControls::inputText("Name", name)) {
        config.setName(name);
        return true;
    }
    return false;
}

/**
 * @brief Draws an engine author input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineAuthor(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string author = config.getAuthor();
    if (ImGuiControls::inputText("Author", author)) {
        config.setAuthor(author);
        return true;
    }
    return false;
}

/**
 * @brief Draws an engine command input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineCommand(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string cmd = config.getCmd();
    if (ImGuiControls::inputText("Command", cmd)) {
        config.setCmd(cmd);
        return true;
    }
    return false;
}

/**
 * @brief Draws an engine directory input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineDirectory(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    std::string dir = config.getDir();
    if (ImGuiControls::inputText("Directory", dir)) {
        config.setDir(dir);
        return true;
    }
    return false;
}

/**
 * @brief Draws a protocol selection combo box
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineProtocol(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    constexpr std::array protocolOptions = {EngineProtocol::Uci, EngineProtocol::XBoard};
    
    std::vector<std::string> labels;
    for (auto option : protocolOptions) {
        labels.push_back(to_string(option));
    }
    
    auto protocolStr = to_string(config.getProtocol());

    if (ImGuiControls::selectionBox("Protocol", protocolStr, labels)) {
        config.setProtocol(parseEngineProtocol(protocolStr));
        return true;
    }
    return false;
}

/**
 * @brief Draws a trace level selection combo box
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineTraceLevel(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    constexpr std::array traceOptions = {TraceLevel::none, TraceLevel::command, TraceLevel::info};
    
    std::vector<std::string> labels;
    for (auto option : traceOptions) {
        labels.push_back(traceToString(option));
    }
    
    auto traceStr = traceToString(config.getTraceLevel());

    if (ImGuiControls::selectionBox("Trace", traceStr, labels)) {
        config.setTraceLevel(traceStr);
        return true;
    }
    return false;
}

/**
 * @brief Draws a restart option selection combo box
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineRestartOption(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    constexpr std::array restartOptions = {RestartOption::EngineDecides, RestartOption::Always, RestartOption::Never};
    
    std::vector<std::string> labels;
    for (auto option : restartOptions) {
        labels.push_back(to_string(option));
    }
    
    auto restartStr = to_string(config.getRestartOption());

    if (ImGuiControls::selectionBox("Restart", restartStr, labels)) {
        config.setRestartOption(parseRestartOption(restartStr));
        return true;
    }
    return false;
}

/**
 * @brief Draws a ponder checkbox control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEnginePonder(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    bool ponder = config.isPonderEnabled();
    if (ImGuiControls::checkbox("Ponder", ponder)) {
        config.setPonder(ponder);
        return true;
    }
    return false;
}

/**
 * @brief Draws a gauntlet checkbox control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineGauntlet(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    return ImGuiControls::checkbox("Gauntlet", config.gauntlet());
}

/**
 * @brief Draws a time control input control
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if the value was changed
 */
inline bool drawEngineTimeControl(EngineConfig& config, bool enabled) {
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
    return false;
}

/**
 * @brief Draws engine-specific options controls using capabilities
 * @param config Engine configuration to modify
 * @param enabled Whether the control is enabled
 * @return True if any option was changed
 */
inline bool drawEngineOptions(EngineConfig& config, bool enabled) {
    if (!enabled) return false;
    
    auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
    auto& capability = capabilities.getCapability(config.getCmd(), config.getProtocol());
    if (!capability) return false;
    
    auto& options = capability->getSupportedOptions();
    if (options.empty()) return false;
    
    bool changed = false;
    ImGui::Separator();
    ImGui::Text("Engine Options:");
    
    auto optionMap = config.getOptionValues();
    for (const auto& option : options) {
        auto it = optionMap.find(option.name);
        std::string value = (it != optionMap.end()) ? it->second : option.defaultValue;
        if (ImGuiControls::engineOptionControl(option, value, 400.0f)) {
            changed = true;
            config.setOptionValue(option.name, value);
        }
    }
    
    return changed;
}

/**
 * @brief Draws read-only engine information
 * @param config Engine configuration to display
 * @param full Whether to show full details (directory and author)
 */
inline void drawEngineReadOnlyInfo(const EngineConfig& config, bool full = false) {
    ImGui::Text("Command: %s", config.getCmd().c_str());
    if (full) ImGui::Text("Directory: %s", config.getDir().c_str());
    ImGui::Text("Protocol: %s", 
        config.getProtocol() == EngineProtocol::Uci ? "UCI" :
        config.getProtocol() == EngineProtocol::XBoard ? "XBoard" : "Unknown");
    if (full) ImGui::Text("Author: %s", config.getAuthor().c_str());
}

} // namespace QaplaWindows::ImGuiEngineControls