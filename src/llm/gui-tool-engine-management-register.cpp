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
        .description = "Lists all chess engines configured in GUI's global engine catalog, "
                        "name + protocol (uci/xboard).",
        .handler = [](const Json::JsonValue&) -> GuiToolResult {
            return GuiToolResult{.success = true, .content = listInstalledEnginesJson()};
        }
    });

    registry.registerTool(GuiToolDefinition{
        .name = "open_add_engine_dialog",
        .description = "Opens GUI's native file picker, user selects one+ chess engine "
                        "executables to add to global engine catalog. User picks file(s) "
                        "themselves -- you have no filesystem access. New engines detected "
                        "(protocol, options, etc.) synchronously before call returns -- result "
                        "already reflects final outcome, don't promise separate future update. "
                        "ENDS YOUR TURN: this result is the last thing you produce, it is shown "
                        "to the user as-is. No further tool call, no reply_to_user afterwards -- "
                        "you won't be asked again.",
        .handler = [](const Json::JsonValue&) -> GuiToolResult {
            auto paths = QaplaWindows::OsDialogs::openFileDialog(true);
            if (paths.empty()) {
                return GuiToolResult{
                    .success = true,
                    .content = "The user cancelled the dialog; no engine was added.",
                    .renderWidget = nullptr,
                    .terminal = true
                };
            }

            auto outcome = addEnginesFromPaths(paths);
            if (!outcome.addedNames.empty()) {
                auto& configuration = QaplaConfiguration::Configuration::instance();
                configuration.setModified();
                // Synchronous and blocking (not the fire-and-forget
                // autoDetect() the "Detect" button uses): detection is
                // normally sub-second per engine, and a tool's result must
                // reflect the *actual* final outcome, not a promise of one
                // -- the model has no way to reliably follow up once its
                // reply for this turn is already sent, and any out-of-band
                // "done" signal (e.g. polling isDetecting()) is not
                // correlated with this turn, so it can reach the chat
                // before or long after this tool result does.
                configuration.getEngineCapabilities().autoDetectSync();
            }

            std::string message;
            if (!outcome.addedNames.empty()) {
                message += "Added and detected: " + joinNames(outcome.addedNames) + ". Ready to use.";
            }
            if (!outcome.duplicateNames.empty()) {
                if (!message.empty()) {
                    message += "\n";
                }
                message += "Already configured (skipped): " + joinNames(outcome.duplicateNames) + ".";
            }
            if (message.empty()) {
                message = "No engines were added.";
            }
            return GuiToolResult{.success = true, .content = message, .renderWidget = nullptr, .terminal = true};
        },
        // Waits on the user picking a file in a native dialog, which can
        // legitimately take much longer than a normal tool call -- the
        // default timeout would abandon the call while the user is still
        // choosing, orphaning its (eventually correct) result. Also covers
        // the synchronous detection below.
        .timeout = std::chrono::minutes(10)
    });
}

} // namespace QaplaLlm
