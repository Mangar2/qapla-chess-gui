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

#include "qapla-tester/time-control.h"
#include <memory>

namespace QaplaConfiguration {
    /**
     * @brief Represents a collection of TimeControl settings for different types.
     */
    struct TimeControlSettings {
        TimeControl blitzTime;       ///< Time control for Blitz games.
        TimeControl tournamentTime; ///< Time control for Tournament games.
        TimeControl timePerMove;    ///< Time control for Time per Move.
        TimeControl fixedDepth;     ///< Time control for Fixed Depth.
        TimeControl nodesPerMove;   ///< Time control for Nodes per Move.
    };

    class Configuration {
    public:
        Configuration();

        /**
         * @brief Gets the time control settings.
         * @return Reference to the current time control settings.
		 */
        const TimeControlSettings& getTimeControlSettings() const {
            return timeControlSettings_;
		}
        TimeControlSettings& getTimeControlSettings() {
            return timeControlSettings_;
        }
        /**
         * @brief Sets the time control settings.
         * @param settings The new time control settings to apply.
		 */
        void setTimeControlSettings(const TimeControlSettings& settings) {
            timeControlSettings_ = settings;
			saveFile(); 
        }

        /**
         * @brief Gets the singleton instance of Configuration.
         * @return Reference to the singleton Configuration instance.
		 */
        static Configuration& instance() {
            static Configuration instance;
            return instance;
		}

        /**
         * @brief Saves the configuration to a file with a safety mechanism.
         */
        void saveFile();

        /**
         * @brief Loads the configuration from a file with a fallback mechanism.
         */
        void loadFile();

    private:
        /**
         * @brief Placeholder for the actual save logic.
         */
         void saveData();

        /**
         * @brief Placeholder for the actual load logic.
         */
        void loadData();
		TimeControlSettings timeControlSettings_; 

        /**
         * @brief Speichert die TimeControl-Einstellungen in den Abschnitt [timecontrol] der Konfigurationsdatei.
         */
        void saveTimeControls();

        static constexpr const char* CONFIG_FILE = "qapla-chess-gui.ini";
        static constexpr const char* BACKUP_FILE = "qapla-chess-gui.ini.bak";

    };

} // namespace QaplaConfiguration
