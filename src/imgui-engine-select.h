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
#include "qapla-tester/ini-file.h"

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
            EngineConfig config;
            bool selected = false;
        };

        /**
         * @brief Options to control which engine properties are editable
         */
        struct Options {
            Options() : allowGauntletEdit(true), allowNameEdit(true), allowPonderEdit(true), 
                       allowTimeControlEdit(true), allowTraceLevelEdit(true), 
                       allowRestartOptionEdit(true), allowEngineOptionsEdit(true) {}
            bool allowGauntletEdit;        ///< Allows editing of the gauntlet option
            bool allowNameEdit;            ///< Allows editing of the engine name
            bool allowPonderEdit;          ///< Allows editing of the ponder option
            bool allowTimeControlEdit;     ///< Allows editing of the time control
            bool allowTraceLevelEdit;      ///< Allows editing of the trace level
            bool allowRestartOptionEdit;   ///< Allows editing of the restart option
            bool allowEngineOptionsEdit;   ///< Allows editing of engine-specific options
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
         * @return true if something changed, false otherwise
         */
        bool draw();

        /**
         * @brief Returns the currently configured engine configurations
         * @return Vector with all configured engine configurations (both selected and deselected)
         */
        const std::vector<EngineConfiguration>& getEngineConfigurations() const;

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

        /**
         * @brief Sets the engine configurations from INI file sections
         * @param sections A list of INI file sections representing the engine configurations
         */
        void setEngineConfiguration(const QaplaHelpers::IniFile::SectionList& sections);

        /**
         * @brief Sets a unique identifier for this selection instance
         * @param id The unique identifier
         */
        void setId(const std::string& id) { id_ = id; }

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
        std::vector<EngineConfiguration>::iterator findEngineConfiguration(const EngineConfig& engineConfig);

        /**
         * @brief Notifies about changes via callback
         */
        void notifyConfigurationChanged();

        /**
         * @brief Informs the configuration singleton about the current engine configurations
         * 
         */
        void updateConfiguration() const;



        Options options_;                                       ///< Current options
        std::string id_ = "unset";                              ///< Unique identifier for the selection instance
        std::vector<EngineConfiguration> engineConfigurations_;     ///< All configured engine configurations (both selected and deselected)
        ConfigurationChangedCallback configurationCallback_;   ///< Callback for changes
    };

} // namespace QaplaWindows
