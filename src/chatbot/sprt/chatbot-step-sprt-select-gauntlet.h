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

#include "../chatbot-step.h"
#include <string>

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to select the gauntlet engine (engine under test) for SPRT tournament.
 * @details This step allows the user to choose which of the two selected engines
 *          should be marked as the gauntlet (engine under test). The other engine
 *          will be used as the comparison baseline.
 */
class ChatbotStepSprtSelectGauntlet : public ChatbotStep {
public:
    ChatbotStepSprtSelectGauntlet() = default;
    ~ChatbotStepSprtSelectGauntlet() override = default;

    [[nodiscard]] std::string draw() override;

private:
    /**
     * @brief Draws the gauntlet selection combo box.
     */
    void drawGauntletSelection();
    
    /**
     * @brief Applies the gauntlet selection to the specified engine index.
     * @param selectedIndex Index of the engine to mark as gauntlet (0 or 1).
     */
    void applyGauntletSelection(int selectedIndex);
    
    /**
     * @brief Finds the current gauntlet engine index.
     * @return Index of the gauntlet engine (0 or 1), or -1 if none is set.
     */
    [[nodiscard]] int findCurrentGauntletIndex() const;
    
    /**
     * @brief Checks if a valid gauntlet selection exists.
     * @return True if exactly one engine has the gauntlet flag set.
     */
    [[nodiscard]] bool hasValidGauntletSelection() const;
};

} // namespace QaplaWindows::ChatBot
