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

#include "gui-tool-status.h"
#include "../tournament-data.h"
#include "../sprt-tournament-data.h"

#include <format>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;

    std::string activityText(bool active, bool starting) {
        if (!active) {
            return "not running";
        }
        return starting ? "starting" : "running";
    }

    GuiToolResult handleGetRunningStatus(const Json::JsonValue&) {
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto& sprtData = QaplaWindows::SprtTournamentData::instance();

        bool tournamentActive = tournamentData.isRunning();
        bool sprtActive = sprtData.isRunning();

        std::string summary;
        if (!tournamentActive && !sprtActive) {
            summary = "Nothing is currently running -- no tournament and no SPRT test.";
        } else if (tournamentActive && sprtActive) {
            summary = "Both a tournament and an SPRT test are currently running.";
        } else if (tournamentActive) {
            summary = "A tournament is currently running; no SPRT test is running.";
        } else {
            summary = "An SPRT test is currently running; no tournament is running.";
        }

        std::string message = std::format(
            "{} Tournament: {}. SPRT test: {}.",
            summary,
            activityText(tournamentActive, tournamentData.isStarting()),
            activityText(sprtActive, sprtData.isStarting()));

        return GuiToolResult{.success = true, .content = message};
    }
}

void registerStatusTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "get_running_status",
        .description = "Reports what's currently running: the classic tournament and the SPRT "
                        "test are checked and reported separately (they run independently, see "
                        "configure_sprt's note on this). Call this whenever the user asks "
                        "broadly whether \"something\"/\"a test\"/\"anything\" is running, or "
                        "asks specifically whether a TOURNAMENT is running -- people often call "
                        "an SPRT test a \"tournament\" informally (it looks the same: engines "
                        "playing games in the background), so checking only "
                        "get_tournament_status could wrongly say nothing is happening while an "
                        "SPRT test is actually active. Prefer this over get_tournament_status/"
                        "get_sprt_status specifically for \"is X running\" questions -- use "
                        "those two instead when the user wants the fuller configuration detail "
                        "of one specific feature they've already named.",
        .handler = handleGetRunningStatus
    });
}

} // namespace QaplaLlm
