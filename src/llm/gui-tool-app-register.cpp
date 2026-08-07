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
            "PGN file source. \"dialog\" (default): native file picker, user picks any PGN "
            "file -- use when user names specific file/path you don't know, or just says "
            "\"open a PGN file\" unqualified. \"tournament\": opens PGN file classic "
            "tournament currently writes to (see configure_tournament's pgn_file) -- use for "
            "\"open the PGN file from the tournament\". \"sprt\": same, for SPRT test's own "
            "PGN output (see configure_sprt's pgn_file). Both need no dialog, path already "
            "known.";
        schema["properties"]["source"] = source;
        return schema;
    }

    GuiToolResult handleOpenPgnFile(const Json::JsonValue& arguments) {
        std::string source = "dialog";
        if (arguments.contains("source") && arguments.at("source").is_string()) {
            source = arguments.at("source").as_string();
        }

        // Only source="dialog" puts a native file picker in front of the user. Every result
        // reached after that point is terminal: the picker steals focus, and by the time the
        // model would speak the dialog is long closed -- exactly the situation where it used
        // to narrate a still-open dialog ("please pick a file now"). Ending the turn on the
        // tool result removes the opportunity entirely, the same way the configure_* file
        // dialogs do (see their dialogOpened flag). Cancelling is terminal too: the outcome
        // does not change that the focus moved and the dialog is gone.
        bool dialogOpened = false;

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
            dialogOpened = true;
            auto paths = QaplaWindows::OsDialogs::openFileDialog(false, {{"PGN files (*.pgn)", "pgn"}});
            if (paths.empty()) {
                return GuiToolResult{.success = true,
                    .content = "The user cancelled the dialog; no PGN file was opened.",
                    .renderWidget = nullptr, .terminal = true};
            }
            path = paths.front();
        }

        if (!std::filesystem::exists(path)) {
            return GuiToolResult{.success = false, .content = "PGN file not found: " + path,
                .renderWidget = nullptr, .terminal = dialogOpened};
        }

        QaplaWindows::StaticCallbacks::message().invokeAll("load_pgn_file:" + path);
        QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_pgn_view");
        return GuiToolResult{.success = true, .content = "Opened PGN file in the Pgn tab: " + path,
            .renderWidget = nullptr, .terminal = dialogOpened};
    }
}

void registerAppTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "close_application",
        .description = "Closes whole Qapla Chess GUI app (same as window's own close button). "
                        "Immediate -- on close/quit/exit request, call directly, no "
                        "confirmation first.",
        .handler = handleCloseApplication
    });

    registry.registerTool(GuiToolDefinition{
        .name = "open_pgn_file",
        .description = "Opens PGN file in Pgn tab, switches to it. Either user picks file via "
                        "native dialog, or directly opens PGN file classic tournament or SPRT "
                        "test is currently writing to (their own \"source\" values) -- see "
                        "\"source\" param for exactly which. source=\"dialog\" ENDS YOUR TURN "
                        "(cancelled or not): that result is the last thing you produce, shown to "
                        "the user as-is, no reply_to_user afterwards. source=\"tournament\"/"
                        "\"sprt\" open no dialog and are followed by a reply as usual.",
        .parametersSchema = buildOpenPgnFileSchema(),
        .handler = handleOpenPgnFile,
        // Waits on the user picking a file in a native dialog (source="dialog") -- see
        // open_add_engine_dialog's identical timeout for why the default would be too short.
        .timeout = std::chrono::minutes(10)
    });
}

} // namespace QaplaLlm
