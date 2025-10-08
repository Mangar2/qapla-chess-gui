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
#include <memory>

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class EngineSetupWindow: public EmbeddedWindow {
    public:
        /**
         * @brief Global engine settings structure.
         */
        struct GlobalEngineSettings {
            bool useGlobalPonder = true;
            bool ponder = false;
            bool useGlobalHash = true;
            uint32_t hashSizeMB = 128;
        };

        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        explicit EngineSetupWindow(bool showGlobalControls = true);
        ~EngineSetupWindow();

        void draw() override;

        /**
         * @brief Returns the list of active engine configurations.
         * Applies global settings if enabled.
         * @return Vector of EngineConfig objects representing the active engines.
		 */
        std::vector<EngineConfig> getActiveEngines() const;
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

        /**
         * @brief Gets the global engine settings.
         * @return The current global engine settings.
         */
        const GlobalEngineSettings& getGlobalSettings() const {
            return globalSettings_;
        }

        /**
         * @brief Sets the global engine settings.
         * @param settings The global engine settings to apply.
         */
        void setGlobalSettings(const GlobalEngineSettings& settings) {
            globalSettings_ = settings;
        }

        /**
         * @brief Enables or disables the global settings controls in the UI.
         * @param enabled True to show global settings controls, false to hide them.
         */
        void setGlobalControlsEnabled(bool enabled) {
            showGlobalControls_ = enabled;
        }

        /**
         * @brief Checks if global settings controls are enabled.
         * @return True if global settings controls are shown.
         */
        bool isGlobalControlsEnabled() const {
            return showGlobalControls_;
        }

    private:
        void drawButtons();
        bool drawGlobalSettings();
        void executeCommand(const std::string &button);
        void drawEngineList();

        std::vector<EngineConfig> activeEngines_;
        GlobalEngineSettings globalSettings_;
        bool showGlobalControls_ = true;

        constexpr static float controlIndent_ = 10.0F;
    };

} // namespace QaplaWindows
