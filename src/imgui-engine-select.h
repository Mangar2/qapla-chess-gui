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
            Options() : allowGauntletEdit(true) {}
            bool allowGauntletEdit;   ///< Allows editing of the gauntlet option
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
         * @brief Returns the currently selected engine configurations
         * @return Vector with the selected configurations
         */
        const std::vector<EngineConfiguration>& getSelectedEngines() const;

        /**
         * @brief Sets the selected engine configurations
         * @param configurations Vector with the configurations to select
         */
        void setSelectedEngines(const std::vector<EngineConfiguration>& configurations);

        /**
         * @brief Sets the callback for configuration changes
         * @param callback The new callback
         */
        void setConfigurationChangedCallback(ConfigurationChangedCallback callback);

        /**
         * @brief Sets the options for available features
         * @param options The new options
         */
        void setOptions(const Options& options);

        /**
         * @brief Returns the current options
         * @return The current options
         */
        const Options& getOptions() const;

    private:
        /**
         * @brief Draws a single engine configuration
         * @param config The engine configuration
         * @param index The index for ImGui IDs
         * @return true if something changed, false otherwise
         */
        bool drawEngineConfiguration(EngineConfiguration& config, int index);

        /**
         * @brief Searches for an engine configuration in the selected engines
         * @param engineConfig The engine configuration to search for
         * @return Iterator to the found configuration or end()
         */
        std::vector<EngineConfiguration>::iterator findSelectedEngine(const EngineConfig& engineConfig);

        /**
         * @brief Notifies about changes via callback
         */
        void notifyConfigurationChanged();

        Options options_;                                       ///< Current options
        std::vector<EngineConfiguration> selectedEngines_;     ///< Selected engine configurations
        ConfigurationChangedCallback configurationCallback_;   ///< Callback for changes
    };

} // namespace QaplaWindows
