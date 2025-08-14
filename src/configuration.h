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
#include <array>

namespace QaplaConfiguration {

    enum selectedTimeControl {
        Blitz = 0,
        Tournament = 1,
        TimePerMove = 2,
        FixedDepth = 3,
        NodesPerMove = 4
	};

    /**
     * @brief Represents a collection of TimeControl settings for different types.
     */
    struct TimeControlSettings {
		bool operator==(const TimeControlSettings& other) const = default;
        const TimeControl& getSelectedTimeControl() const {
            switch (selected) {
                case Blitz: return blitzTime;
                case Tournament: return tournamentTime;
                case TimePerMove: return timePerMove;
                case FixedDepth: return fixedDepth;
                case NodesPerMove: return nodesPerMove;
                default:
                    return blitzTime;
            }
            throw std::runtime_error("Invalid selected time control");
		}
        TimeControl blitzTime;       ///< Time control for Blitz games.
        TimeControl tournamentTime; ///< Time control for Tournament games.
        TimeControl timePerMove;    ///< Time control for Time per Move.
        TimeControl fixedDepth;     ///< Time control for Fixed Depth.
        TimeControl nodesPerMove;   ///< Time control for Nodes per Move.
		selectedTimeControl selected = Blitz; 
        
        static constexpr std::array<const char*, 5> timeControlStrings{
            "Blitz", "Tournament", "TimePerMove", "FixedDepth", "NodesPerMove"
        };

        inline std::string getSelectionString() {
            if (static_cast<size_t>(selected) >= timeControlStrings.size()) {
                throw std::out_of_range("Invalid selectedTimeControl value");
            }
            return timeControlStrings[static_cast<size_t>(selected)];
        }

        inline void setSelectionFromString(const std::string& selection) {
            for (size_t i = 0; i < timeControlStrings.size(); ++i) {
                if (selection == timeControlStrings[i]) {
                    selected = static_cast<selectedTimeControl>(i);
                    return;
                }
            }
            throw std::invalid_argument("Invalid time control selection: " + selection);
		}
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
            if (timeControlSettings_ == settings) return;
            timeControlSettings_ = settings;
			changed_ = true;
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
		 * @brief Autosaves the configuration if it has changed since the last save and enough 
         * time has passed since last save.
         */
        void autosave();

        /**
         * @brief Saves the configuration to a file with a safety mechanism.
         */
        void saveFile();

        /**
         * @brief Loads the configuration from a file with a fallback mechanism.
         */
        void loadFile();

    private:
        bool changed_ = false;
		uint64_t lastSaveTimestamp_ = 0;

        /**
         * @brief Writes a map in the format a=b\n to an existing ofstream.
         * @param outFile The already opened ofstream to which the map will be written.
         * @param map The map containing key-value pairs to be saved.
         * @throws std::runtime_error If an error occurs during writing.
         */
        void saveMap(std::ofstream& outFile, const std::map<std::string, std::string>& map);

        /**
         * @brief Placeholder for the actual save logic.
		 * @param out The output stream to write the configuration data to.
         */
         void saveData(std::ofstream& out);

        /**
         * @brief Placeholder for the actual load logic.
         */
        void loadData(std::ifstream& in);
		TimeControlSettings timeControlSettings_; 

        /**
        * @brief Processes a specific section from the configuration file.
        * @param section The name of the section being processed.
        * @param keyValueMap A map containing key-value pairs for the section.
        * @throws std::runtime_error If an error occurs while processing the section.
        */
        void processSection(const std::string& section, const std::map<std::string, std::string>& keyValueMap);

        /**
         * @brief Parses the "timecontrol" section and updates the corresponding TimeControl settings.
         * @param keyValueMap A map containing key-value pairs for the "timecontrol" section.
         * @throws std::runtime_error If an error occurs while parsing the timecontrol section.
         */
        void parseTimeControl(const std::map<std::string, std::string>& keyValueMap);

        /**
         * @brief Parses the "board" section and updates the time control settings.
         * @param keyValueMap A map containing key-value pairs for the "board" section.
		 */
        void parseBoard(const std::map<std::string, std::string>& keyValueMap);

        /**
         * @brief Speichert die TimeControl-Einstellungen in den Abschnitt [timecontrol] der Konfigurationsdatei.
         */
        void saveTimeControls(std::ofstream& out);

        static constexpr const char* CONFIG_FILE = "qapla-chess-gui.ini";
        static constexpr const char* BACKUP_FILE = "qapla-chess-gui.ini.bak";

    };

} // namespace QaplaConfiguration
