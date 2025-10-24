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

#include <string>
#include <unordered_map>

/**
 * @brief Manages tutorial progress for different topics in the Chess GUI
 * 
 * The Tutorial class tracks which tutorial topics have been completed
 * and provides methods to check completion status and restart tutorials.
 */
class Tutorial {
public:
    /**
     * @brief Tutorial topics available in the application
     */
    enum class Topic {
        Snackbar,
        EngineSetup,
        EngineWindow,
        // Future topics can be added here
        Count
    };

    /**
     * @brief Gets the singleton instance of the Tutorial manager
     * @return Reference to the singleton Tutorial instance
     */
    static Tutorial& instance() {
        static Tutorial instance;
        return instance;
    }

    /**
     * @brief Checks if a tutorial topic is completed
     * @param topic The tutorial topic to check
     * @return True if the topic is completed, false otherwise
     */
    bool isCompleted(Topic topic) const;

    /**
     * @brief Restarts a tutorial topic from the beginning
     * @param topic The tutorial topic to restart
     */
    void restartTopic(Topic topic);

    /**
     * @brief Gets the display name for a tutorial topic
     * @param topic The tutorial topic
     * @return Display name of the topic
     */
    static std::string getTopicName(Topic topic);

    /**
     * @brief Gets the completion threshold for a specific topic
     * @param topic The tutorial topic
     * @return The counter value at which the topic is considered complete
     */
    static uint32_t getCompletionThreshold(Topic topic);

    /**
     * @brief Loads all tutorial counters from the configuration file
     */
    void loadConfiguration();

    /**
     * @brief Saves all tutorial counters to the configuration file
     */
    void saveConfiguration() const;

private:
    Tutorial() = default;
};
