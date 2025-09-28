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

#include "qapla-tester/time-control.h"
#include "qapla-tester/ini-file.h"

#include <memory>
#include <array>
#include <unordered_map>

namespace QaplaConfiguration {

    enum selectedTimeControl {
        Blitz = 0,
        tcTournament = 1,
        TimePerMove = 2,
        FixedDepth = 3,
        NodesPerMove = 4
	};

	/**
     * @brief Represents a collection of TimeControl settings for different types.
     */
    struct TimeControlSettings {
        TimeControlSettings() {
            blitzTime.addTimeSegment({0, 1 * 60 * 1000, 0}); // Default Blitz: 1 min + 0 sec increment
            tournamentTime.addTimeSegment({10 * 60 * 1000, 0, 40}); // Default Tournament: 10 min + 0 sec increment + 40 moves
            timePerMove.setMoveTime(10 * 1000); // Default Time per Move: 10 sec per move
            fixedDepth.setDepth(10); // Default Fixed Depth: 10 plies
            nodesPerMove.setNodes(100000); // Default Nodes per Move: 100,000 nodes
        }
		bool operator==(const TimeControlSettings& other) const = default;
        const TimeControl& getSelectedTimeControl() const {
            switch (selected) {
                case Blitz: return blitzTime;
                case tcTournament: return tournamentTime;
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

        std::string getSelectionString() {
            if (static_cast<size_t>(selected) >= timeControlStrings.size()) {
                throw std::out_of_range("Invalid selectedTimeControl value");
            }
            return timeControlStrings[static_cast<size_t>(selected)];
        }

        void setSelectionFromString(const std::string& selection) {
            for (size_t i = 0; i < timeControlStrings.size(); ++i) {
                if (selection == timeControlStrings[i]) {
                    selected = static_cast<selectedTimeControl>(i);
                    return;
                }
            }
            throw std::invalid_argument("Invalid time control selection: " + selection);
		}

        /**
         * @brief Checks if the current time control settings are valid.
         * @return True if the selected time control is valid, false otherwise.
         */
        bool isValid() const {
            return getSelectedTimeControl().isValid();
        }
    };

    class Configuration : public QaplaHelpers::Autosavable {
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
			setModified();
        }

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

        /**
         * @brief Parses the "timecontrol" section and updates the corresponding TimeControl settings.
         * @param section The section being processed.
         * @throws std::runtime_error If an error occurs while parsing the timecontrol section.
         */
        void parseTimeControl(const QaplaHelpers::IniFile::Section& section);

        /**
         * @brief Parses the "board" section and updates the time control settings.
         * @param keyValueMap A map containing key-value pairs for the "board" section.
		 */
        void parseBoard(const QaplaHelpers::IniFile::Section& section);

        /**
         * @brief Speichert die TimeControl-Einstellungen in den Abschnitt [timecontrol] der Konfigurationsdatei.
         */
        void saveTimeControls(std::ofstream& out);

        EngineCapabilities engineCapabilities_;
        TimeControlSettings timeControlSettings_;
        QaplaHelpers::ConfigData configData_;

        static constexpr const char* CONFIG_FILE = "qapla-chess-gui.ini";

    };

} // namespace QaplaConfiguration
