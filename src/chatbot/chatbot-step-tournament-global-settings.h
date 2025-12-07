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

namespace QaplaWindows {
    class ImGuiEngineGlobalSettings;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to configure global engine settings (hash, time control).
 * Supports both standard tournaments and SPRT tournaments.
 */
class ChatbotStepTournamentGlobalSettings : public ChatbotStep {
public:
    explicit ChatbotStepTournamentGlobalSettings(EngineSelectContext type = EngineSelectContext::Standard);
    ~ChatbotStepTournamentGlobalSettings() override = default;

    [[nodiscard]] std::string draw() override;

private:
    EngineSelectContext type_;
    bool showMoreOptions_ = false;  ///< Show advanced options

    /**
     * @brief Gets the global settings for the tournament.
     * @return Reference to the global settings.
     */
    [[nodiscard]] ImGuiEngineGlobalSettings& getGlobalSettings();
};

} // namespace QaplaWindows::ChatBot
