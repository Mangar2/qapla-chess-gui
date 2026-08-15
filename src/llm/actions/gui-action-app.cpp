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

// App-level actions act via QaplaWindows::StaticCallbacks::message() -- the same pub/sub bus the
// classic chatbot's own "Switch to ... View" buttons use -- because neither the main window
// (GLFWwindow*) nor the PGN tab (ImGuiGameList, not a singleton) is otherwise reachable from
// src/llm/.

#include "gui-action-app.h"
#include "../../tournament-data.h"
#include "../../sprt-tournament-data.h"
#include "../../os-dialogs.h"
#include "../../callback-manager.h"

#include <filesystem>
#include <string>

namespace QaplaLlm::Actions {

ActionResult closeApplication() {
    // Same effect as the OS window-close button (see the message subscription registered in
    // qapla-chess-gui.cpp's runApp()).
    QaplaWindows::StaticCallbacks::message().invokeAll("quit_application");
    return succeeded("Closing the application.");
}

ActionResult openPgnFile(PgnSource source) {
    // Only PgnSource::AskUser puts a native file picker in front of the user, and every result
    // reached after that point ends the turn -- see this action's doc comment.
    bool dialogShown = false;

    std::string path;
    if (source == PgnSource::TournamentOutput) {
        path = QaplaWindows::TournamentData::instance().pgnConfig().file;
        if (path.empty()) {
            return failed("The tournament has no PGN output file configured.");
        }
    } else if (source == PgnSource::SprtOutput) {
        path = QaplaWindows::SprtTournamentData::instance().tournamentPgn().pgnOptions().file;
        if (path.empty()) {
            return failed("The SPRT test has no PGN output file configured.");
        }
    } else {
        dialogShown = true;
        auto paths = QaplaWindows::OsDialogs::openFileDialog(false, {{"PGN files (*.pgn)", "pgn"}});
        if (paths.empty()) {
            return ActionResult{.ok = true,
                .text = "The user cancelled the dialog; no PGN file was opened.",
                .widget = nullptr, .endsTurn = true};
        }
        path = paths.front();
    }

    if (!std::filesystem::exists(path)) {
        return ActionResult{.ok = false, .text = "PGN file not found: " + path, .widget = nullptr,
            .endsTurn = dialogShown};
    }

    QaplaWindows::StaticCallbacks::message().invokeAll("load_pgn_file:" + path);
    QaplaWindows::StaticCallbacks::message().invokeAll("switch_to_pgn_view");
    return ActionResult{.ok = true, .text = "Opened PGN file in the Pgn tab: " + path,
        .widget = nullptr, .endsTurn = dialogShown};
}

} // namespace QaplaLlm::Actions
