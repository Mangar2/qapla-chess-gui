/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */
#pragma once

#include "qapla-engine/types.h"
#include "qapla-tester/move-record.h"

#include "imgui-board.h"

#include <optional>
#include <unordered_set>
#include <utility>
#include <string>
#include <imgui.h>

namespace QaplaWindows
{

    class BoardWindow : public ImGuiBoard
    {
    public:
        explicit BoardWindow() {}
        ~BoardWindow() override = default;
        
        /**
         * @brief Draws the control buttons for the board.
         * 
         * @param status The current status of the game (e.g., "Running", "Stopped").
         * @return The command associated with the clicked button, or an empty string if no button was clicked.
         * 
         */
        std::string drawButtons(const std::string& status);
    private:
        /**
         * @brief Draws the setup mode buttons and executes the associated commands.
         */
        std::string drawSetupButtons();
        
        /**
         * @brief Draws the board control buttons and returns the command of the clicked button.
         * 
         * @param status The current status of the game (e.g., "Running", "Stopped").
         * @return The command associated with the clicked button, or an empty string if no button was clicked.
         */
        std::string drawBoardButtons(const std::string& status);

        /**
         * @brief Executes the setup command associated with the clicked button.
         * 
         * @param command The command to execute.
         */
        void executeSetupCommand(const std::string& command);
        bool setupMode_ = false;
        
    };

}
