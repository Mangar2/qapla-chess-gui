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
 * Supports tournaments, SPRT tournaments, EPD analysis, and interactive boards via callback.
 */
class ChatbotStepSelectEngines : public ChatbotStep {
public:
    /**
     * @brief Callback function type that returns a reference to engine selection.
     * The callback may return nullptr if the target object no longer exists.
     */
    using EngineSelectProvider = std::function<ImGuiEngineSelect*()>;

    /**
     * @brief Constructs with an engine select provider callback.
     * @param provider Callback to retrieve the engine selection.
     * @param contextName Name to display in UI (e.g., "tournament", "EPD analysis", "board").
     */
    explicit ChatbotStepSelectEngines(
        EngineSelectProvider provider,
        const char* contextName = "tournament");
    ~ChatbotStepSelectEngines() override;

    [[nodiscard]] std::string draw() override;

private:
    EngineSelectProvider provider_;   ///< Callback for engine selection
    const char* contextName_;         ///< Context name for UI text
    bool showMoreOptions_ = false;    ///< Show advanced options

    /**
     * @brief Gets the engine selection from the provider.
     * @return Pointer to engine selection, or nullptr if target no longer exists.
     */
    [[nodiscard]] ImGuiEngineSelect* getEngineSelect();
};

} // namespace QaplaWindows::ChatBot
