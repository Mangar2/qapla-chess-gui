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
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "chatbot-step-tournament-load.h"
#include "tournament-data.h"
#include "sprt-tournament-data.h"
#include "imgui-controls.h"
#include "os-dialogs.h"
#include <imgui.h>

namespace QaplaWindows::ChatBot {

ChatbotStepTournamentLoad::ChatbotStepTournamentLoad(TournamentType type)
    : type_(type) {}

std::string ChatbotStepTournamentLoad::draw() {
    if (finished_) {
        return "Tournament loaded successfully.";
    }

    std::vector<std::pair<std::string, std::string>> filters;
    if (type_ == TournamentType::Sprt) {
        filters = { {"Qapla SPRT Tournament Files", "*.qsprt"}, {"All Files", "*.*"} };
    } else {
        filters = { {"Qapla Tournament Files", "*.qtour"}, {"All Files", "*.*"} };
    }

    auto selectedPath = OsDialogs::openFileDialog(false, filters);
    if (!selectedPath.empty() && !selectedPath[0].empty()) {
        if (type_ == TournamentType::Sprt) {
            SprtTournamentData::instance().loadTournament(selectedPath[0]);
        } else {
            TournamentData::instance().loadTournament(selectedPath[0]);
        }
        finished_ = true;
        return "start";
    }
    finished_ = true;
    return "stop";
}

} // namespace QaplaWindows::ChatBot
