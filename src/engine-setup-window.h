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
#include "imgui-button.h"
#include "imgui-engine-select.h"
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
         * @brief Indicates whether the window should be highlighted.
         * @return true if no engines are configured, false otherwise.
         */
        bool highlighted() const override;

        /**
         * @brief Returns the list of active engine configurations.
         * Applies global settings if enabled.
         * @return Vector of EngineConfig objects representing the active engines.
		 */
        std::vector<QaplaTester::EngineConfig> getActiveEngines() const;
        /**
         * @brief Sets the list of active engine configurations.
         * 
		 * It searches for matching EngineConfig objects in the EngineConfigManager and
		 * sets the configurations from the configManager that match the provided engines.
		 * Matching is done by comparing the command and protocol of each EngineConfig.
         * 
         * @param engines Vector of EngineConfig objects to set as active engines.
         */
        void setMatchingActiveEngines(const std::vector<QaplaTester::EngineConfig>& engines);

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

        /**
         * @brief Sets the direct edit mode for the engine selection.
         * @param enabled True to enable direct edit mode (no "Engines" header, can expand without selection),
         *                false for normal mode (with "Engines" header, selection required for expansion).
         */
        void setDirectEditMode(bool enabled) {
            engineSelect_.getOptions().directEditMode = enabled;
        }

        /**
         * @brief Sets whether multiple selection of the same engine is allowed.
         * @param enabled True to allow multiple selection (shows "Selected" and "Available" sections),
         *                false for single selection mode (flat list).
         */
        void setAllowMultipleSelection(bool enabled) {
            engineSelect_.getOptions().allowMultipleSelection = enabled;
        }

        /**
         * @brief Gets the current tutorial counter value.
         * @return The tutorial counter value.
         */
        static uint32_t getTutorialCounter();

        /**
         * @brief Resets the tutorial counter to restart the tutorial.
         */
        static void resetTutorialCounter();

        /**
         * @brief Finishes the tutorial without showing any snackbars.
         */
        static void finishTutorial();

        /**
         * @brief Loads the tutorial configuration from the configuration data.
         */
        static void loadTutorialConfiguration();

    private:
        void drawButtons();
        bool drawGlobalSettings();
        void executeCommand(const std::string &button);
        void drawEngineList();
        
        /**
         * @brief Gets the button state for a specific button based on current context.
         * @param button The button identifier ("Add", "Remove", "Detect")
         * @return The appropriate button state
         */
        QaplaButton::ButtonState getButtonState(const std::string& button) const;

        /**
         * @brief Checks and manages the engine setup tutorial progression.
         */
        void checkTutorialProgression();

        /**
         * @brief Updates the configuration data with the current tutorial counter.
         */
        static void updateTutorialConfiguration();

        /**
         * @brief Increments the tutorial counter.
         */
        static void showNextTutorialStep();

        ImGuiEngineSelect engineSelect_;
        GlobalEngineSettings globalSettings_;
        bool showGlobalControls_ = true;

        static inline uint32_t engineSetupTutorialCounter_ = 0;

        constexpr static float controlIndent_ = 10.0F;
    };

} // namespace QaplaWindows
