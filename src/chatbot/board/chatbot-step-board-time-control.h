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
#include <functional>

namespace QaplaWindows {
    class TimeControlWindow;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to configure time control settings for an interactive board.
 * Provides simplified view (Blitz only) by default with option to show all settings.
 */
class ChatbotStepBoardTimeControl : public ChatbotStep {
public:
    /**
     * @brief Callback function type that returns a reference to TimeControlWindow.
     * The callback may return nullptr if the target object no longer exists.
     */
    using TimeControlProvider = std::function<TimeControlWindow*()>;

    /**
     * @brief Constructs with a time control provider callback.
     */
    explicit ChatbotStepBoardTimeControl(TimeControlProvider provider);
    
    ~ChatbotStepBoardTimeControl() override = default;

    [[nodiscard]] std::string draw() override;

private:
    TimeControlProvider provider_;    ///< Callback for time control window
    bool showMoreOptions_ = false;    ///< Show all time control options

    /**
     * @brief Gets the time control window from the provider.
     * @return Pointer to time control window, or nullptr if target no longer exists.
     */
    [[nodiscard]] TimeControlWindow* getTimeControlWindow();
};

} // namespace QaplaWindows::ChatBot
