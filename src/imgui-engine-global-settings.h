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

#include "tutorial.h"

#include "ini-file.h"

#include <string>
#include <functional>

class Tutorial;

namespace QaplaTester
{
    class EngineConfig;
}

namespace QaplaWindows {

    struct DrawControlOptions {
        float controlWidth = 150.0F;      ///< Width of input controls
        float controlIndent = 10.0F;      ///< Indentation for controls
    };

    /**
     * @brief Standalone class for global engine settings in ImGui
     * 
     * This class manages UI controls and data for global engine settings that apply
     * across multiple engines. It can be embedded in various dialogs (e.g., tournament,
     * engine setup) to provide consistent global configuration options.
     */
    class ImGuiEngineGlobalSettings {
    public:
        /**
         * @brief Global engine settings data structure
         */
        struct GlobalConfiguration {
            bool useGlobalHash = true;      ///< Whether to use a global hash size setting
            uint32_t hashSizeMB = 32;       ///< Hash size in MB (1-64000)
            
            bool useGlobalPonder = true;    ///< Whether to use a global ponder setting
            bool ponder = false;             ///< Whether pondering is enabled globally
            
            bool useGlobalTrace = true;     ///< Whether to use a global trace level setting
            std::string traceLevel = "command"; ///< Global trace level ("none", "all", "command")
            
            bool useGlobalRestart = true;   ///< Whether to use a global restart option
            std::string restart = "auto";    ///< Global restart option ("auto", "on", "off")
        };

        /**
         * @brief Time control settings data structure
         */
        struct TimeControlSettings {
            std::string timeControl = "60.0+0.0";  ///< Time control string
            std::vector<std::string> predefinedOptions = {
                "Custom", "10.0+0.02", "20.0+0.02", "50.0+0.10", "60.0+0.20"
            };
        };

        /**
         * @brief Options to control which settings are displayed
         */
        struct Options {
            // Needed due to clang not accepting default member initializers when used as 
            // default parameters in the same class.
            Options() : 
                showHash(true), showPonder(true), showTrace(true), 
                showRestart(true), showUseCheckboxes(true), alwaysOpen(false)
            {}
            
            bool showHash = true;          ///< Show hash size control
            bool showPonder = true;        ///< Show ponder control
            bool showTrace = true;         ///< Show trace level control
            bool showRestart = true;       ///< Show restart option control
            bool showUseCheckboxes = true; ///< Show "use global" checkboxes (if false, controls are always global)
            bool alwaysOpen = false;
        };

        /**
         * @brief Callback type that is called when the configuration changes
         * @param globalSettings The current global settings
         */
        using ConfigurationChangedCallback = std::function<void(const GlobalConfiguration&)>;

        /**
         * @brief Callback type that is called when the time control changes
         * @param timeControlSettings The current time control settings
         */
        using TimeControlChangedCallback = std::function<void(const TimeControlSettings&)>;

        /**
         * @brief Constructor
         * @param callback Optional callback that is called when configuration changes
         */
        explicit ImGuiEngineGlobalSettings(ConfigurationChangedCallback callback = nullptr);

        /**
         * @brief Destructor
         */
        ~ImGuiEngineGlobalSettings() = default;

        /**
         * @brief Draws the global engine settings interface
         * @param controls Options for control dimensions (width, indent)
         * @param options Options to control which settings are displayed (if not provided, uses stored options)
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return true if any setting was changed, false otherwise
         */
        bool drawGlobalSettings(DrawControlOptions controls = DrawControlOptions{},
            const Options& options = Options{},
            const Tutorial::TutorialContext& tutorialContext = Tutorial::TutorialContext{});

        /**
         * @brief Draws the time control interface
         * @param controls Options for control dimensions (width, indent)
         * @param blitz Whether to use blitz mode (no hours in time input)
         * @param alwaysOpen Whether the section should always be open
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return true if any setting was changed, false otherwise
         */
        bool drawTimeControl(DrawControlOptions controls = DrawControlOptions{}, bool blitz = false,
            bool alwaysOpen = false,
            const Tutorial::TutorialContext& tutorialContext = Tutorial::TutorialContext{});

        /**
         * @brief Returns the current global settings
         * @return Reference to the current global settings
         */
        const GlobalConfiguration& getGlobalConfiguration() const { return globalSettings_; }

        /**
         * @brief Sets the global settings
         * @param globalSettings The new global settings
         */
        void setGlobalConfiguration(const GlobalConfiguration& globalSettings);

        /**
         * @brief Returns the current time control settings
         * @return Reference to the current time control settings
         */
        const TimeControlSettings& getTimeControlSettings() const { return timeControlSettings_; }

        /**
         * @brief Sets the time control settings
         * @param timeControlSettings The new time control settings
         */
        void setTimeControlSettings(const TimeControlSettings& timeControlSettings);

        /**
         * @brief Sets the callback for configuration changes
         * @param callback The new callback
         */
        void setConfigurationChangedCallback(ConfigurationChangedCallback callback);

        /**
         * @brief Sets the callback for time control changes
         * @param callback The new callback
         */
        void setTimeControlChangedCallback(TimeControlChangedCallback callback);

        /**
         * @brief Sets the global settings from INI file sections
         * @param sections A list of INI file sections representing the global settings
         */
        void setGlobalConfiguration(const QaplaHelpers::IniFile::SectionList& sections);

        /**
         * @brief Sets the time control settings from INI file sections
         * @param sections A list of INI file sections representing the time control settings
         */
        void setTimeControlConfiguration(const QaplaHelpers::IniFile::SectionList& sections);

        /**
         * @brief Sets a unique identifier for this instance
         * @param id The unique identifier
         */
        void setId(const std::string& id) { id_ = id; }

        /**
         * @brief Applies global settings to an engine configuration
         * @param engine The engine configuration to modify
         * @param globalSettings The global settings to apply
         * @param timeControlSettings The time control settings to apply
         */
        static void applyGlobalConfig(QaplaTester::EngineConfig& engine, 
                                      const GlobalConfiguration& globalSettings, 
                                      const TimeControlSettings& timeControlSettings);

    private:
        /**
         * @brief Notifies about configuration changes via callback
         */
        void notifyConfigurationChanged();

        /**
         * @brief Notifies about time control changes via callback
         */
        void notifyTimeControlChanged();

        /**
         * @brief Updates the configuration singleton with current global settings
         */
        void updateConfiguration() const;

        /**
         * @brief Updates the time control options configuration
         */
        void updateTimeControlConfiguration() const;

        /**
         * @brief Loads hash settings from an INI section
         * @param section The INI section to load from
         * @param settings The settings structure to update
         */
        static void loadHashSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings);

        /**
         * @brief Loads ponder settings from an INI section
         * @param section The INI section to load from
         * @param settings The settings structure to update
         */
        static void loadPonderSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings);

        /**
         * @brief Loads trace settings from an INI section
         * @param section The INI section to load from
         * @param settings The settings structure to update
         */
        static void loadTraceSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings);

        /**
         * @brief Loads restart settings from an INI section
         * @param section The INI section to load from
         * @param settings The settings structure to update
         */
        static void loadRestartSettings(const QaplaHelpers::IniFile::Section& section, GlobalConfiguration& settings);

        /**
         * @brief Draws the hash size control
         * @param controlWidth Width of the input control
         * @param showUseCheckboxes Whether to show the "use global" checkbox
         * @param tutorialContext Tutorial context for annotations
         * @return true if modified, false otherwise
         */
        bool drawHashControl(float controlWidth, bool showUseCheckboxes, 
            const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the restart option control
         * @param controlWidth Width of the input control
         * @param showUseCheckboxes Whether to show the "use global" checkbox
         * @param tutorialContext Tutorial context for annotations
         * @return true if modified, false otherwise
         */
        bool drawRestartControl(float controlWidth, bool showUseCheckboxes,
            const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the trace level control
         * @param controlWidth Width of the input control
         * @param showUseCheckboxes Whether to show the "use global" checkbox
         * @param tutorialContext Tutorial context for annotations
         * @return true if modified, false otherwise
         */
        bool drawTraceControl(float controlWidth, bool showUseCheckboxes,
            const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the ponder control
         * @param controlWidth Width of the input control
         * @param showUseCheckboxes Whether to show the "use global" checkbox
         * @param tutorialContext Tutorial context for annotations
         * @return true if modified, false otherwise
         */
        bool drawPonderControl(float controlWidth, bool showUseCheckboxes,
            const Tutorial::TutorialContext& tutorialContext);

        std::string id_ = "unset";                              ///< Unique identifier for this instance
        GlobalConfiguration globalSettings_;                         ///< Current global settings
        TimeControlSettings timeControlSettings_;               ///< Current time control settings
        ConfigurationChangedCallback configurationCallback_;    ///< Callback for changes
        TimeControlChangedCallback timeControlCallback_;        ///< Callback for time control changes
    };

} // namespace QaplaWindows
