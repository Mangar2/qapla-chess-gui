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

#include "engine-capabilities.h"
#include "autosavable.h"
#include "callback-manager.h"

#include "base-elements/ini-file.h"

#include <memory>
#include <array>
#include <unordered_map>

namespace QaplaConfiguration {

    class Configuration : public QaplaHelpers::Autosavable {
    public:
        Configuration();

        /**
         * @brief Gets the singleton instance of Configuration.
         * @return Reference to the singleton Configuration instance.
		 */
        static Configuration& instance() {
            static Configuration instance;
            return instance;
		}

        // Inherited from Autosavable: autosave(), saveFile(), loadFile(), setModified()
        
        const EngineCapabilities& getEngineCapabilities() const {
            return engineCapabilities_;
        }

        EngineCapabilities& getEngineCapabilities() {
            return engineCapabilities_;
		}

        /**
         * @brief Gets the configuration data manager.
         * @return Reference to the configuration data manager.
         */
        const QaplaHelpers::ConfigData& getConfigData() const {
            return configData_;
        }
        
        QaplaHelpers::ConfigData& getConfigData() {
            return configData_;
        }

        void autosave() override {
            if (getConfigData().getDirty()) {
                setModified();
            }
            Autosavable::autosave();
        }

        /**
         * @brief Loads the logger configuration from the configuration data
         */
        static void loadLoggerConfiguration();

        /**
         * @brief Updates the configuration data with the current logger settings
         */
        static void updateLoggerConfiguration();

        /**
         * @brief Loads the language configuration from the configuration data
         */
        static void loadLanguageConfiguration();

        /**
         * @brief Updates the configuration data with the current language settings
         * @param language The language code to save
         */
        static void updateLanguageConfiguration(const std::string& language);

        /**
         * @brief Gets the Remote Desktop mode setting
         * @return true if Remote Desktop optimization is enabled
         */
        static bool isRemoteDesktopMode();

        /**
         * @brief Sets the Remote Desktop mode setting
         * @param enabled true to enable Remote Desktop optimization
         */
        static void setRemoteDesktopMode(bool enabled);

    protected:
        /**
         * @brief Saves configuration data to the output stream.
         * Overrides Autosavable::saveData.
		 * @param out The output stream to write the configuration data to.
         */
        void saveData(std::ofstream& out) override;

        /**
         * @brief Loads configuration data from the input stream.
         * Overrides Autosavable::loadData.
         * @param in The input stream to read the configuration data from.
         */
        void loadData(std::ifstream& in) override;

    private:

        /**
        * @brief Processes a specific section from the configuration file.
        * @param section The section being processed.
        * @return True if the section was processed successfully, false otherwise.
        * @throws std::runtime_error If an error occurs while processing the section.
        */
        bool processSection(const QaplaHelpers::IniFile::Section& section);

        EngineCapabilities engineCapabilities_;
        QaplaHelpers::ConfigData configData_;

        static constexpr const char* CONFIG_FILE = "qapla-chess-gui.ini";

        std::unique_ptr<QaplaWindows::Callback::UnregisterHandle> saveCallbackHandle_;

    };

} // namespace QaplaConfiguration
