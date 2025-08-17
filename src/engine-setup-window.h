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

#include "qapla-tester/engine-config.h"
#include "embedded-window.h"
#include "board-data.h"
#include <memory>

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class EngineSetupWindow: public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        EngineSetupWindow();
        ~EngineSetupWindow();

        void draw() override;

        /**
         * @brief Returns the list of active engine configurations.
         * @return Vector of EngineConfig objects representing the active engines.
		 */
        const std::vector<EngineConfig>& getActiveEngines() const {
            return activeEngines_;
        }
        /**
         * @brief Sets the list of active engine configurations.
         * 
		 * It searches for matching EngineConfig objects in the EngineConfigManager and
		 * sets the configurations from the configManager that match the provided engines.
		 * Matching is done by comparing the command and protocol of each EngineConfig.
         * 
         * @param engines Vector of EngineConfig objects to set as active engines.
         */
        void setMatchingActiveEngines(const std::vector<EngineConfig>& engines);

    private:

        void drawButtons();
		void drawEngineList();

        /**
         * @brief Draws a collapsible section for editing a single engine configuration.
         * @param config Reference to the engine configuration to edit.
         * @param index Index of the engine, used to generate unique ImGui IDs.
		 * @return first bool indicates if the configuration was changed, second bool indicates if the section was selected.
         */
        std::tuple<bool, bool> drawEngineConfigSection(EngineConfig& config, int index, bool selected);

        /**
         * @brief Draws the options for a given engine configuration.
         * @param config Reference to the engine configuration.
         * @param inputWidth Width of the input fields for options.
         * @return True if any option was changed, false otherwise.
		 */
        bool drawOptions(EngineConfig& config, float inputWidth);
        std::vector<EngineConfig> activeEngines_;
    };

} // namespace QaplaWindows
