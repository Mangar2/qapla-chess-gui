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
#include "imgui-engine-global-settings.h"
#include <memory>

namespace QaplaWindows {

    class ImGuiTable;

    class EngineSetupWindow: public EmbeddedWindow {
    public:
        explicit EngineSetupWindow(bool showGlobalControls = true);
        ~EngineSetupWindow();

        /**
         * @brief Renders the contents of the engine setup window.
         */
        void draw() override;

        /**
         * @brief Sets the unique identifier to be used to store and load configuration settings.
         * @param id The unique identifier string.
         */
        void setId(const std::string& id) {
            id_ = id;
            engineSelect_.setId(id);
            globalSettings_.setId(id);
        }

        /**
         * @brief Indicates whether the tab control of the window should be highlighted.
         * 
         * This can be used to signal special attention or status in the UI.
         * 
         * @return true if the tab control is highlighted, false otherwise
         */
        bool highlighted() const override;

        /**
         * @brief Returns the list of active engine configurations.
         * Applies global settings if enabled.
         * @return Vector of EngineConfig objects representing the active engines.
		 */
        std::vector<QaplaTester::EngineConfig> getActiveEngines() const;
        
        /**
         * @brief Sets the active engines based on the provided configurations.
         * @param engines Vector of EngineConfig objects to set as active.
         */
        void setMatchingActiveEngines(const std::vector<QaplaTester::EngineConfig>& engines);

        /** 
         * @brief Gets the global engine settings.
         * @return The current global settings.
         */
        const ImGuiEngineGlobalSettings::GlobalConfiguration& getGlobalConfiguration() const {
            return globalSettings_.getGlobalSettings();
        }

        /** 
         * @brief Sets the global engine settings.
         * @param settings The global settings to apply.
         */
        void setGlobalConfiguration(const ImGuiEngineGlobalSettings::GlobalConfiguration& settings) {
            globalSettings_.setGlobalSettings(settings);
        }

        /** 
         * @brief Sets the global engine settings from INI file sections.
         * @param sections A list of INI file sections representing the global settings.
         */
        void setGlobalConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
             globalSettings_.setGlobalConfiguration(sections);
         }

        /**
         * @brief Sets the engine configurations from INI file sections
         * @param sections A list of INI file sections representing the engine configurations
         */
        void setEnginesConfiguration(const QaplaHelpers::IniFile::SectionList& sections) {
            engineSelect_.setEnginesConfiguration(sections);
        }

        /** 
         * @brief Sets whether global controls are shown.
         * @param enabled True to show global controls, false to hide.
         */
        void setGlobalControlsEnabled(bool enabled) {
            showGlobalControls_ = enabled;
        }

        /** 
         * @brief Checks if global controls are enabled.
         * @return True if global controls are shown, false otherwise.
         */
        bool isGlobalControlsEnabled() const {
            return showGlobalControls_;
        }

        /** 
         * @brief Sets the direct edit mode for the engine selection.
         * @param enabled True to enable direct edit mode, false to disable.
         */
        void setDirectEditMode(bool enabled) {
            engineSelect_.getOptions().directEditMode = enabled;
        }

        /**
         * @brief Sets whether multiple engine selection is allowed.
         * @param enabled True to allow multiple selection, false to disallow.
         */
        void setAllowMultipleSelection(bool enabled) {
            engineSelect_.getOptions().allowMultipleSelection = enabled;
        }

        /**
         * @brief Gets the current tutorial counter.
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
        ImGuiEngineGlobalSettings globalSettings_;
        bool showGlobalControls_ = true;
        std::string id_;

        static inline uint32_t engineSetupTutorialCounter_ = 0;

        constexpr static float controlIndent_ = 10.0F;
    };

} // namespace QaplaWindows
