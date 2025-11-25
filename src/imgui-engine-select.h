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

#include "engine-config.h"
#include "ini-file.h"

#include <vector>
#include <functional>

namespace QaplaWindows {

    /**
     * @brief Standalone class for engine selection and configuration in ImGui
     */
    class ImGuiEngineSelect {
    public:
        /**
         * @brief Structure for an engine configuration with selection status
         */
        struct EngineConfiguration {
            QaplaTester::EngineConfig config;
            bool selected = false;
            std::string originalName;  ///< Original name from config or user-modified name, used for disambiguation reset
        };

        /**
         * @brief Options to control which engine properties are editable
         */
        struct Options {
            Options() {};
            bool allowProtocolEdit = false;      ///< Allows editing of the engine protocol
            bool allowGauntletEdit = true;        ///< Allows editing of the gauntlet option
            bool allowNameEdit = true;            ///< Allows editing of the engine name
            bool allowPonderEdit = true;          ///< Allows editing of the ponder option
            bool allowScoreFromWhitePovEdit = true;///< Allows editing of the score from white POV option
            bool allowTimeControlEdit = true;     ///< Allows editing of the time control
            bool allowTraceLevelEdit = true;      ///< Allows editing of the trace level
            bool allowRestartOptionEdit = true;   ///< Allows editing of the restart option
            bool allowEngineOptionsEdit = true;   ///< Allows editing of engine-specific options
            bool allowMultipleSelection = false;   ///< Allows selecting the same engine multiple times
            bool directEditMode = false;           ///< Direct edit mode: skips "Engines" header, engines can be edited without prior selection, changes saved via callback
            bool enginesDefaultOpen = false;       ///< If true, the "Engines" collapsing header is default open
            bool allowEngineConfiguration = true;  ///< Allows editing of the engine configuration
        };

        /**
         * @brief Callback type that is called when the configuration changes
         * @param configurations Vector with all current engine configurations
         */
        using ConfigurationChangedCallback = std::function<void(const std::vector<EngineConfiguration>&)>;

        /**
         * @brief Constructor
         * @param options Options to control available features
         * @param callback Callback that is called when changes occur
         */
        ImGuiEngineSelect(const Options& options = Options{}, 
                         ConfigurationChangedCallback callback = nullptr);

        /**
         * @brief Destructor
         */
        ~ImGuiEngineSelect() = default;

        /**
         * @brief Draws the engine selection interface
         * @param highlight Whether to highlight the "Engines" collapsing header with a red dot
         * @return true if something changed, false otherwise
         */
        bool draw(bool highlight = false);

        /**
         * @brief Returns the currently configured engine configurations
         * @return Vector with all configured engine configurations (both selected and deselected)
         */
        const std::vector<EngineConfiguration>& getEngineConfigurations() const;

        /**
         * @brief Returns only the selected engine configurations
         * @return Vector with selected engine configurations
         */
        std::vector<EngineConfiguration> getSelectedEngines() const;

        /**
         * @brief Sets the configured engine configurations
         * @param configurations Vector with all configured engine configurations
         */
        void setEngineConfigurations(const std::vector<EngineConfiguration>& configurations);

        /**
         * @brief Sets the callback for configuration changes
         * @param callback The new callback
         */
        void setConfigurationChangedCallback(ConfigurationChangedCallback callback);

        /**
         * @brief Sets the options for available features
         * @param options The new options
         */
        void setOptions(const Options& options) { options_ = options; }

        /**
         * @brief Returns the current options
         * @return The current options
         */
        const Options& getOptions() const { return options_; }
        Options& getOptions() { return options_; }

        /**
         * @brief Sets the engine configurations from INI file sections
         * @param sections A list of INI file sections representing the engine configurations
         */
        void setEnginesConfiguration(const QaplaHelpers::IniFile::SectionList& sections);

        /**
         * @brief Sets a unique identifier for this selection instance
         * @param id The unique identifier
         */
        void setId(const std::string& id) { id_ = id; }

        /**
         * @brief Sets whether to always show the engines without collapsing
         * @param alwaysShow True to always show, false to allow collapsing
         */
        void setAlwaysShowEngines(bool alwaysShow) { alwaysShowEngines_ = alwaysShow; }

        /**
         * @brief Opens a file dialog to add engines and optionally selects them
         * @param select If true, the engines will be selected after adding
         * @return Vector of paths of the added engines
         */
        std::vector<std::string> addEngines(bool select = false);

        /**
         * @brief Checks if all engines have been auto-detected
         * @return true if all engines are detected, false otherwise
         */
        [[nodiscard]] static bool areAllEnginesDetected();

        /**
         * @brief Draws the available engines section (without checkboxes)
         * @return true if something changed, false otherwise
         */
        bool drawAvailableEngines();

    private:
        /**
         * @brief Draws a single engine configuration
         * @param config The engine configuration
         * @param index The index for ImGui IDs
         * @return true if something changed, false otherwise
         */
        bool drawEngineConfiguration(EngineConfiguration& config, int index);

        /**
         * @brief Searches for an engine configuration in the configured engines
         * @param engineConfig The engine configuration to search for
         * @return Iterator to the found configuration or end()
         */
        std::vector<EngineConfiguration>::iterator findEngineConfiguration(const QaplaTester::EngineConfig& engineConfig);
        
        /**
         * @brief Finds the first deselected engine configuration that matches the given base engine
         * @param engineConfig The base engine configuration to match against
         * @return Iterator to the found configuration or end()
         */
        std::vector<EngineConfiguration>::iterator findDeselectedEngineConfiguration(const QaplaTester::EngineConfig& engineConfig);

        /**
         * @brief Draws the selected engines section (with checkboxes)
         * @return true if something changed, false otherwise
         */
        bool drawSelectedEngines();

        /**
         * @brief Draws all engines in single selection mode
         * @return true if something changed, false otherwise
         */
        bool drawAllEngines();

        /**
         * @brief Notifies about changes via callback
         */
        void notifyConfigurationChanged();

        /**
         * @brief Updates unique display names for all configured engines
         */
        void updateUniqueDisplayNames();

        /**
         * @brief Informs the configuration singleton about the current engine configurations
         * 
         */
        void updateConfiguration() const;

        /**
         * @brief Resets all engine names to their original names before disambiguation
         */
        void resetNamesToOriginal();

        /**
         * @brief Cleans up engine configurations that are no longer available
         * @return true if any engines were removed, false otherwise
         */
        bool cleanupNonAvailableEngines();

        bool alwaysShowEngines_ = true;                         ///< Always show the engines, disable collapsing header
        Options options_;                                       ///< Current options
        std::string id_ = "unset";                              ///< Unique identifier for the selection instance
        std::vector<EngineConfiguration> engineConfigurations_;     ///< All configured engine configurations (both selected and deselected)
        ConfigurationChangedCallback configurationCallback_;   ///< Callback for changes
    };

} // namespace QaplaWindows
