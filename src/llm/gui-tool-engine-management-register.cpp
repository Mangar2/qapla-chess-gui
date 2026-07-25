/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2026 Volker Böhm
 */

// Split out of gui-tool-engine-management.cpp so the unit-tests target can
// link the pure logic (addEnginesFromPaths/listInstalledEnginesJson)
// without dragging in the GUI stack: OsDialogs and Configuration transitively
// pull in ImGui/GLFW headers the unit-tests target has no include paths for.

#include "gui-tool-engine-management.h"
#include "../os-dialogs.h"
#include "../configuration.h"

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;

    Json::JsonValue noArgsSchema() {
        auto schema = Json::JsonValue::object();
        schema["type"] = "object";
        schema["properties"] = Json::JsonValue::object();
        return schema;
    }

    std::string joinNames(const std::vector<std::string>& names) {
        std::string joined;
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (i > 0) {
                joined += ", ";
            }
            joined += names[i];
        }
        return joined;
    }
}

void registerEngineManagementTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "list_installed_engines",
        .description = "Lists all chess engines currently configured in the GUI's global "
                        "engine catalog, with their name and protocol (uci/xboard).",
        .parametersSchema = noArgsSchema(),
        .handler = [](const Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{.success = true, .content = listInstalledEnginesJson()};
        }
    });

    registry.registerTool(GuiToolDefinition{
        .name = "open_add_engine_dialog",
        .description = "Opens the GUI's native file picker so the user can select one or more "
                        "chess engine executables to add to the global engine catalog. The user "
                        "chooses the file(s) themselves -- you have no filesystem access. Call "
                        "list_installed_engines afterwards to confirm what was actually added.",
        .parametersSchema = noArgsSchema(),
        .handler = [](const Json::JsonValue&) -> GuiToolResult {
            auto paths = QaplaWindows::OsDialogs::openFileDialog(true);
            if (paths.empty()) {
                return GuiToolResult{
                    .success = true,
                    .content = "The user cancelled the dialog; no engine was added."
                };
            }

            auto outcome = addEnginesFromPaths(paths);
            if (!outcome.addedNames.empty()) {
                QaplaConfiguration::Configuration::instance().setModified();
            }

            std::string message;
            if (!outcome.addedNames.empty()) {
                message += "Added engines: " + joinNames(outcome.addedNames) + ". ";
            }
            if (!outcome.duplicateNames.empty()) {
                message += "Already configured (skipped): " + joinNames(outcome.duplicateNames) + ". ";
            }
            if (message.empty()) {
                message = "No engines were added.";
            }
            return GuiToolResult{.success = true, .content = message};
        }
    });
}

} // namespace QaplaLlm
