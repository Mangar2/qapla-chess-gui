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

#include "embedded-window.h"
#include "imgui-engine-select.h"

#include <engine-handling/engine-config.h>

#include <memory>

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the SPRT tournament configuration and execution window.
     */
    class SprtTournamentWindow: public EmbeddedWindow {
    public:
        SprtTournamentWindow() = default;
        ~SprtTournamentWindow() = default;

        /**
         * @brief Renders the contents of the SPRT tournament window.
         * @details Draws the window including buttons, input controls, and progress display.
         */
        void draw() override;

    private:
        /**
         * @brief Draws the control buttons for the SPRT tournament.
         * @details Renders buttons like Run, Stop, Clear, Load, and Save.
         */
        static void drawButtons();

        /**
         * @brief Executes a command based on the button pressed.
         * @param button The name of the button that was pressed.
         */
        static void executeCommand(const std::string &button);

        /**
         * @brief Draws the input controls for SPRT tournament configuration.
         * @details Renders all configuration sections including engines, opening, and SPRT settings.
         * @return True if any configuration value was changed, false otherwise.
         */
        static bool drawInput();

        /**
         * @brief Draws the progress indicator for the running SPRT tournament.
         * @details Displays a progress bar showing the current state of the tournament.
         */
        static void drawProgress();
    };

} // namespace QaplaWindows
