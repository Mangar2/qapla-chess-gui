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
#include "imgui-engine-list.h"
#include <memory>

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class EngineWindow : public ImGuiEngineList {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        EngineWindow();
        virtual ~EngineWindow() override;

        /**
         * @brief Renders the engine window and its components.
         * This method should be called within the main GUI rendering loop.
         * @return A pair containing the engine ID and command string if an action was triggered, otherwise empty strings.
         */
        std::pair<std::string, std::string> draw() override;

        /**
         * @brief Gets the current tutorial counter.
         * @return The tutorial counter value.
         */
        static uint32_t getTutorialCounter();

        /**
         * @brief Sets the tutorial counter value.
         * @param value The new counter value.
         */
        static void setTutorialCounter(uint32_t value);

        /**
         * @brief Resets the tutorial counter to restart the tutorial.
         */
        static void resetTutorialCounter();

        /**
         * @brief Finishes the tutorial without showing any snackbars.
         */
        static void finishTutorial();

    private:

        static constexpr float areaWidth = 65.0F;

        /**
         * @brief Draws the configuration button area for all engines.
         * @param noEnginesSelected Flag indicating if there are no engines selected.
         * @param enginesAvailable Flag indicating if there are any engines available.
         * @return The required indent for the button area.
         */
        static std::string drawConfigButtonArea(bool noEnginesSelected, bool enginesAvailable);

        /**
         * @brief Checks and manages the engine window tutorial progression.
         * @param configCommandIssued Whether the Config button was clicked.
         * @param activeEngines The currently active engines.
         */
        void checkTutorialProgression(bool configCommandIssued, const std::vector<QaplaTester::EngineConfig>& activeEngines);

        /**
         * @brief Increments the tutorial counter and shows the next tutorial step.
         */
        static void showNextTutorialStep();

        static inline uint32_t engineWindowTutorialCounter_ = 0;

    };

} // namespace QaplaWindows
