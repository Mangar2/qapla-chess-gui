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
#include "../epd-data.h"

#include <cctype>
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
        auto& epdData = QaplaWindows::EpdData::instance();

        bool tournamentActive = tournamentData.isRunning();
        bool sprtActive = sprtData.isRunning();
        bool epdActive = epdData.isRunning() || epdData.isStarting();

        std::vector<std::string> active;
        if (tournamentActive) {
            active.emplace_back("a tournament");
        }
        if (sprtActive) {
            active.emplace_back("an SPRT test");
        }
        if (epdActive) {
            active.emplace_back("an EPD analysis");
        }

        std::string summary;
        if (active.empty()) {
            summary = "Nothing is currently running -- no tournament, no SPRT test, no EPD analysis.";
        } else {
            std::string joined;
            if (active.size() == 1) {
                joined = active.front();
            } else if (active.size() == 2) {
                joined = active[0] + " and " + active[1];
            } else {
                joined = active[0] + ", " + active[1] + ", and " + active[2];
            }
            joined[0] = static_cast<char>(std::toupper(joined[0]));
            summary = joined + " currently running.";
        }

        std::string message = std::format(
            "{} Tournament: {}. SPRT test: {}. EPD analysis: {}.",
            summary,
            activityText(tournamentActive, tournamentData.isStarting()),
            activityText(sprtActive, sprtData.isStarting()),
            activityText(epdActive, epdData.isStarting()));

        return GuiToolResult{.success = true, .content = message};
    }
}

void registerStatusTools(GuiToolRegistry& registry) {
    registry.registerTool(GuiToolDefinition{
        .name = "get_running_status",
        .description = "Reports what's currently running: classic tournament, SPRT test, EPD "
                        "analysis checked/reported separately (run independently, see "
                        "configure_sprt's/configure_epd's notes). Call whenever user asks "
                        "broadly if \"something\"/\"a test\"/\"anything\" running, or "
                        "specifically if TOURNAMENT running -- people often call SPRT test or "
                        "EPD analysis \"tournament\" informally (all look same: engines running "
                        "in background), so checking only get_tournament_status could wrongly "
                        "say nothing happening while SPRT test or EPD analysis actually active. "
                        "Prefer this over get_tournament_status/get_sprt_status/get_epd_status "
                        "for \"is X running\" questions -- use those instead when user wants "
                        "fuller config detail of one specific feature already named.",
        .handler = handleGetRunningStatus
    });
}

} // namespace QaplaLlm
