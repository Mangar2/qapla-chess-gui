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
#include "imgui-button.h"

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

        static inline uint32_t tutorialProgress_ = 0; ///< Progress counter for the tutorial

    private:
        /**
         * @brief Draws the setup mode buttons and executes the associated commands.
         */
        std::string drawSetupButtons();

        /**
         * @brief Draws a button in setup mode and returns whether it was clicked.
         * 
         * @param button The button identifier (e.g., "New", "Clear").
         * @param label The label to display on the button.
         * @param buttonSize The size of the button.
         * @return True if the button was clicked, false otherwise.
         */
        bool drawSetupButton(
            const std::string& button, const std::string& label,
            const ImVec2& buttonSize) const;

        /**
         * @brief Determines the button state (normal, highlighted, disabled) based on the current setup mode and button type.
         * 
         * @param button The button identifier
         * @return The ButtonState for the specified button.
         */
        QaplaButton::ButtonState getSetupButtonState(const std::string& button) const;

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

        /**
         * @brief Determines the button state including tutorial highlights.
         * 
         * @param button The button identifier
         * @param status The current game status
         * @return The ButtonState for the specified button
         */
        QaplaButton::ButtonState getBoardButtonState(const std::string& button, const std::string& status) const;

        /**
         * @brief Calculates which buttons fit on screen and which go into More menu.
         * 
         * @param allButtons All available buttons
         * @param availableWidth Available screen width
         * @param buttonWidth Width of a single button including spacing
         * @param status Current game status for button states
         * @return Pair of visible buttons and More menu commands
         */
        std::pair<std::vector<std::string>, std::vector<QaplaButton::PopupCommand>> 
        splitButtonsForResponsiveLayout(
            const std::vector<std::string>& allButtons,
            float availableWidth,
            float buttonWidth,
            const std::string& status) const;

        /**
         * @brief Draws all visible buttons in the button bar.
         * 
         * @param visibleButtons Buttons to display
         * @param startPos Starting position for drawing
         * @param buttonSize Size of each button
         * @param totalSize Total size including label
         * @param status Current game status
         * @return The clicked button name or empty string
         */
        std::string drawVisibleButtons(
            const std::vector<std::string>& visibleButtons,
            ImVec2 startPos,
            ImVec2 buttonSize,
            ImVec2 totalSize,
            const std::string& status);

        /**
         * @brief Checks if any command in the More menu is highlighted.
         * 
         * @param moreCommands Commands in the More menu
         * @return true if any command is highlighted
         */
        bool hasHighlightedCommand(const std::vector<QaplaButton::PopupCommand>& moreCommands) const;

        /**
         * @brief Advances the tutorial based on button clicks and game state.
         * 
         * @param clickedButton The button that was clicked
         * @param status The current game status
         */
        void showNextBoardTutorialStep(const std::string& clickedButton, const std::string& status);

        bool setupMode_ = false;
        
        // Tutorial highlight flags
        bool highlightPlay_ = false;
        bool highlightStop_ = false;
        bool highlightAnalyze_ = false;
        bool highlightAuto_ = false;

        uint32_t tutorialSubStep_ = 0; ///< Sub-step counter for more granular tutorial tracking
    };

}
