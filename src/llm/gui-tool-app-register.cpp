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

#include "gui-tool-app.h"
#include "../tournament-data.h"
#include "../sprt-tournament-data.h"
#include "../os-dialogs.h"
#include "../callback-manager.h"

#include <chrono>
#include <filesystem>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;

    GuiToolResult handleCloseApplication(const Json::JsonValue&) {
        // Same effect as the OS window-close button (see the message subscription registered
        // in qapla-chess-gui.cpp's runApp()): the main loop exits normally on its next check
        // and runs its usual shutdown/save sequence, so this is a clean quit, not an abrupt exit.
        QaplaWindows::StaticCallbacks::message().invokeAll("quit_application");
        return GuiToolResult{.success = true, .content = "Closing the application."};
    }

    Json::JsonValue buildOpenPgnFileSchema() {
        auto schema = noArgsToolSchema();
        auto source = Json::JsonValue::object();
        source["type"] = "string";
        auto enumValues = Json::JsonValue::array();
        enumValues.push_back("dialog");
        enumValues.push_back("tournament");
        enumValues.push_back("sprt");
        source["enum"] = enumValues;
        source["description"] =
            "Where to get the PGN file from. \"dialog\" (default if omitted): opens a native "
            "file picker so the user chooses any PGN file themselves -- use this whenever the "
            "user names a specific file/path you don't already know, or just says \"open a PGN "
            "file\" without qualifying it. \"tournament\": opens the PGN file the classic "
            "tournament is currently configured to write its games to (see "
            "configure_tournament's pgn_file) -- use this for phrases like \"open the PGN file "
            "from the tournament\". \"sprt\": the same, but for the SPRT test's own PGN output "
            "file (see configure_sprt's pgn_file). These two never need a dialog since the path "
            "is already known.";
        schema["properties"]["source"] = source;
        return schema;
    }

    GuiToolResult handleOpenPgnFile(const Json::JsonValue& arguments) {
        std::string source = "dialog";
        if (arguments.contains("source") && arguments.at("source").is_string()) {
            source = arguments.at("source").as_string();
        }

        std::string path;
        if (source == "tournament") {
            path = QaplaWindows::TournamentData::instance().pgnConfig().file;
            if (path.empty()) {
                return GuiToolResult{
                    .success = false,
                    .content = "The tournament has no PGN output file configured (see configure_tournament's pgn_file)."
                };
            }
        } else if (source == "sprt") {
            path = QaplaWindows::SprtTournamentData::instance().tournamentPgn().pgnOptions().file;
            if (path.empty()) {
                return GuiToolResult{
                    .success = false,
                    .content = "The SPRT test has no PGN output file configured (see configure_sprt's pgn_file)."
                };
            }
        } else {
            auto paths = QaplaWindows::OsDialogs::openFileDialog(false, {{"PGN files (*.pgn)", "pgn"}});
            if (paths.empty()) {
                return GuiToolResult{.success = true, .content = "The user cancelled the dialog; no PGN file was opened."};
            }
            path = paths.front();
        }

        if (!std::filesystem::exists(path)) {
            return GuiToolResult{.success = false, .content = "PGN file not found: " + path};
        }

        QaplaWindows::StaticCallbacks::message().invokeAll("load_pgn_file:" + path);
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_pgn_view");
        return GuiToolResult{.success = true, .content = "Opened PGN file in the Pgn tab: " + path};
    }
}

void registerAppTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "close_application",
        .description = "Closes the whole Qapla Chess GUI application (the same as clicking the "
                        "window's own close button). Does exactly what it says immediately -- "
                        "when the user asks to close/quit/exit the application, call this "
                        "directly, do not ask for confirmation first.",
        .handler = handleCloseApplication
    });

    registry.registerTool(GuiToolDefinition{
        .name = "open_pgn_file",
        .description = "Opens a PGN file for viewing in the Pgn tab and switches to it. Can "
                        "either let the user pick any file via a native dialog, or directly "
                        "open the PGN file the classic tournament or the SPRT test is currently "
                        "writing its games to (their own \"source\" values) -- see this tool's "
                        "\"source\" parameter for exactly which.",
        .parametersSchema = buildOpenPgnFileSchema(),
        .handler = handleOpenPgnFile,
        // Waits on the user picking a file in a native dialog (source="dialog") -- see
        // open_add_engine_dialog's identical timeout for why the default would be too short.
        .timeout = std::chrono::minutes(10)
    });
}

} // namespace QaplaLlm
