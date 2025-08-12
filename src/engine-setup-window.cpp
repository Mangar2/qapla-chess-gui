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

#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-config.h"
#include "qapla-tester/engine-worker-factory.h"

#include "imgui.h"

#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;


EngineSetupWindow::EngineSetupWindow(std::shared_ptr<BoardData> boardData)
    : boardData_(std::move(boardData))
{
}

EngineSetupWindow::~EngineSetupWindow() = default;

bool EngineSetupWindow::drawEngineConfigSection(EngineConfig& config, int index) {
    std::string headerLabel = config.getName().empty()
        ? std::format("Engine {}", index + 1)
        : std::format("{}##engineHeader{}", config.getName(), index);
    bool changed = false;
    if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID(index);

        if (auto name = ImGuiControls::inputText("Name", config.getName())) {
            config.setName(*name);
            changed = true;
        }

        if (auto cmd = ImGuiControls::inputText("Command", config.getCmd())) {
            config.setCmd(*cmd);
            changed = true;
        }

        if (auto dir = ImGuiControls::inputText("Directory", config.getDir())) {
            config.setDir(*dir);
            changed = true;
        }

        static const char* protocolItems[] = { "uci", "xboard" };
        int protocolIndex = static_cast<int>(config.getProtocol());
        if (ImGui::Combo("Protocol", &protocolIndex, protocolItems, IM_ARRAYSIZE(protocolItems))) {
            config.setProtocol(static_cast<EngineProtocol>(protocolIndex));
            changed = true;
        }

        static const char* traceItems[] = { "none", "all", "commands" };
        int traceIndex = static_cast<int>(config.getTraceLevel());
        if (ImGui::Combo("Trace", &traceIndex, traceItems, IM_ARRAYSIZE(traceItems))) {
            config.setTraceLevel(traceItems[traceIndex]);
            changed = true;
        }

        static const char* restartItems[] = { "auto", "on", "off" };
        int restartIndex = static_cast<int>(config.getRestartOption());
        if (ImGui::Combo("Restart", &restartIndex, restartItems, IM_ARRAYSIZE(restartItems))) {
            config.setRestartOption(static_cast<RestartOption>(restartIndex));
            changed = true;
        }

        ImGui::PopID();
    }
    return changed;
}



void EngineSetupWindow::draw() {
    auto& configManager = EngineWorkerFactory::getConfigManagerMutable();
    auto configs = configManager.getAllConfigs();
    int index = 0;
    for (auto& config : configs) {
        if (drawEngineConfigSection(config, index)) {
            configManager.addOrReplaceConfig(config);
        }
		index++;
	}
}

