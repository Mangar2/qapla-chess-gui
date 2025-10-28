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
#include "interactive-board-window.h"
#include "imgui-engine-select.h"
#include <memory>

namespace QaplaWindows
{

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class EpdWindow : public EmbeddedWindow
    {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        EpdWindow();
        ~EpdWindow();

        void draw() override;
        
        /**
         * @brief Indicates whether the EPD tab should be highlighted for tutorial.
         * @return true if tutorial is active and not completed
         */
        bool highlighted() const override;

        /**
         * @brief Advances the EPD tutorial based on user actions.
         * @param clickedButton The button that was clicked (empty string for state checks)
         */
        void showNextEpdTutorialStep(const std::string& clickedButton);

        static inline uint32_t tutorialProgress_ = 0; ///< Progress counter for EPD tutorial
        static inline std::string highlightedButton_ = ""; ///< Button to highlight for tutorial
        
    private:
        static std::string drawButtons();
        static void executeCommand(const std::string &button);
        static void drawInput();
        static void drawProgress();

    };

} // namespace QaplaWindows
