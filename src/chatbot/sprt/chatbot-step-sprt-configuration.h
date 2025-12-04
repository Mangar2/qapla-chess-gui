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
 * @brief Step to configure SPRT (Sequential Probability Ratio Test) parameters.
 * 
 * Configures:
 * - Elo Lower (H0): null hypothesis threshold
 * - Elo Upper (H1): alternative hypothesis threshold  
 * - Alpha: Type I error rate (false positive)
 * - Beta: Type II error rate (false negative)
 * - Max Games: Maximum games before inconclusive termination
 */
class ChatbotStepSprtConfiguration : public ChatbotStep {
public:
    ChatbotStepSprtConfiguration() = default;
    ~ChatbotStepSprtConfiguration() override = default;

    [[nodiscard]] std::string draw() override;

private:
    /**
     * @brief Draws the SPRT configuration controls.
     */
    void drawConfiguration();

    /**
     * @brief Validates the SPRT configuration.
     * @return true if configuration is valid, false otherwise.
     */
    [[nodiscard]] bool isConfigurationValid() const;

    bool showMoreOptions_ = false;  ///< Show advanced options (Alpha, Beta)
};

} // namespace QaplaWindows::ChatBot
