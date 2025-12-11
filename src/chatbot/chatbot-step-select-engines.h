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
 * @author GitHub Copilot
 * @copyright Copyright (c) 2025 GitHub Copilot
 */

#pragma once

#include "chatbot-step.h"
#include <string>
#include <functional>

namespace QaplaWindows {
    class ImGuiEngineSelect;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to select engines from the list of available engines.
 * Supports tournaments, SPRT tournaments, and EPD analysis.
 */
class ChatbotStepSelectEngines : public ChatbotStep {
public:
    using EngineSelectFactory = std::function<ImGuiEngineSelect&()>;

    explicit ChatbotStepSelectEngines(
        EngineSelectContext context = EngineSelectContext::Standard);
    ~ChatbotStepSelectEngines() override;

    [[nodiscard]] std::string draw() override;

private:
    EngineSelectContext context_;
    bool showMoreOptions_ = false;

    /**
     * @brief Gets the engine selection based on context and type.
     * @return Reference to the engine selection.
     */
    [[nodiscard]] ImGuiEngineSelect& getEngineSelect();
    
    /**
     * @brief Gets the context name for UI text.
     * @return "tournament" or "EPD analysis"
     */
    [[nodiscard]] const char* getContextName() const;
};

} // namespace QaplaWindows::ChatBot
