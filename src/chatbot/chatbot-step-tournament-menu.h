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

#pragma once

#include "chatbot-step.h"
#include "chatbot-step-tournament-stop-running.h"
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to ask the user what they want to do with the tournament.
 * 
 * Options: New tournament, Save tournament, Load tournament
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentMenu : public ChatbotStep {
public:
    explicit ChatbotStepTournamentMenu(TournamentType type = TournamentType::Standard);

    [[nodiscard]] std::string draw() override;

private:
    TournamentType type_;
    bool saved_ = false;

    /**
     * @brief Clears the tournament data.
     */
    void clearTournament();

    /**
     * @brief Saves the tournament to a file.
     * @param path The file path to save to.
     */
    void saveTournament(const std::string& path);

    /**
     * @brief Gets the file filter for the save dialog.
     * @return The file filter pair (description, extension).
     */
    [[nodiscard]] std::pair<std::string, std::string> getFileFilter() const;

    /**
     * @brief Gets the tournament name for display.
     * @return The tournament name string.
     */
    [[nodiscard]] const char* getTournamentName() const;
};

} // namespace QaplaWindows::ChatBot
