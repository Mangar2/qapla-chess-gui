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

#include "snackbar.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace QaplaWindows {

/**
 * @brief Manages tutorial progress for different topics in the Chess GUI
 */
class Tutorial {
public:
    /**
     * @brief Tutorial context for UI elements with highlighting and input annotations
     */
    struct TutorialContext {
        bool highlight = false;                             ///< Whether to highlight the section
        std::unordered_map<std::string, std::string> annotations; ///< Input line annotations (key=label, value=hint text)
        
        void clear() {
            highlight = false;
            annotations.clear();
        }
    };

    struct Message {
        std::string text;
        std::string success{};
        SnackbarManager::SnackbarType type;
        bool sticky = false;
        bool waitForUserInput = false;
    };

    enum class TutorialName {
        Snackbar,
        EngineSetup,
        BoardEngines,
        BoardWindow,
        BoardCutPaste,
        Epd,
        Tournament,
        Count
    };

    struct Entry {
        TutorialName name;
        std::string displayName;
        std::vector<Message> messages;
        std::function<uint32_t&()> getProgressCounter;
        bool autoStart = false;

        uint32_t counter = 0;
        uint32_t successCounter = 0;
        void reset() {
            counter = 0;
            successCounter = 0;
            getProgressCounter() = 0;
        }
        void finish() {
            if (!completed()) {
                counter = static_cast<uint32_t>(messages.size()) + 1;
                getProgressCounter() = counter;
            }
        }
        bool completed() const {
            return counter > messages.size();
        }
        void showNextMessage() {
            if (counter < messages.size() && counter < getProgressCounter()) {
                const auto& msg = messages[counter];
                SnackbarManager::instance().showTutorial(msg.text, msg.type, msg.sticky);
            }
            // Beeing finished is an additional state after all messages shown
            if (counter < getProgressCounter()) {
                ++counter;
            }
        }
        void showSuccessMessage() {
            auto msgIndex = successCounter;
            if (msgIndex < messages.size() && msgIndex < getProgressCounter()) {
                const auto& msg = messages[msgIndex];
                if (!msg.success.empty()) {
                    SnackbarManager::instance().showTutorial(msg.success, msg.type, false);
                }
            }
            if (msgIndex < getProgressCounter()) {
                ++successCounter;
            }
        }
    };

    /**
     * @brief Starts a tutorial 
     * @param name The name of the tutorial topic
     */
    void startTutorial(TutorialName name);

    /**
     * @brief Shows the current tutorial step without advancing the counter
     * @param name The name of the tutorial topic
     */
    void showLastTutorialStep(TutorialName name);

    /**
     * @brief Shows the next tutorial step for a given topic
     * @param topicName The name of the tutorial topic
     * @param mayWaitForUserInput If true, waits for user input before, if the supports it
     */
    void requestNextTutorialStep(TutorialName name, bool mayWaitForUserInput = true);

    /**
     * @brief Finishes a tutorial topic without showing further messages
     * @param topic The tutorial topic name to finish
     */
    void finishTutorial(TutorialName name);

    /**
     * @brief Restarts a tutorial topic from the beginning
     * @param name The tutorial topic to restart
     */
    void restartTutorial(TutorialName name);

    /**
     * @brief Resets all tutorials to their initial state
     */
    void resetAll();

    /**
     * @brief Sets a tutorial entry at the specified position
     * @param entry The tutorial entry to set
     */
    void setEntry(const Entry& entry) {
        entries_[toIndex(entry.name)] = entry;
    }

    /**
     * @brief Gets the singleton instance of the Tutorial manager
     * @return Reference to the singleton Tutorial instance
     */
    static Tutorial& instance() {
        static Tutorial instance;
        return instance;
    }

    /**
     * @brief Converts a tutorial name string to TutorialName enum.
     * @param name The tutorial name as string.
     * @return The corresponding TutorialName, or Count if not found.
     */
    [[nodiscard]] static TutorialName stringToTutorialName(const std::string& name);

    /**
     * @brief Loads all tutorial counters from the configuration file
     */
    void loadConfiguration();

    /**
     * @brief Saves all tutorial counters to the configuration file
     */
    void saveConfiguration() const;

    /**
     * @brief Gets all tutorial entries
     * @return Reference to the array of tutorial entries
     */
    auto& getEntries() {
        return entries_;
    }

    /**
     * @brief Gets a specific tutorial entry
     * @param name The tutorial name
     * @return Reference to the entry
     */
    Entry& getEntry(TutorialName name) {
        return entries_[toIndex(name)];
    }

    const Entry& getEntry(TutorialName name) const {
        return entries_[toIndex(name)];
    }

    /**
     * @brief Checks if tutorial steps should wait for user input
     * @return true if currently waiting for user input, false otherwise
     */
    bool doWaitForUserInput() const {
        return waitForUserInput_;
    }

private:
    Tutorial() = default;
    
    // Verwende std::array statt std::vector für O(1) Zugriff über enum
    std::array<Entry, static_cast<size_t>(TutorialName::Count)> entries_;

    /**
     * @brief Converts TutorialName enum to array index
     * @param name The tutorial name
     * @return The corresponding array index
     */
    static constexpr size_t toIndex(TutorialName name) {
        return static_cast<size_t>(name);
    }

    /**
     * @brief Checks if all tutorials before the given one are completed
     * @param name The tutorial name to check
     * @return true if all preceding tutorials are completed, false otherwise
     */
    bool allPrecedingCompleted(TutorialName name) const;

    /**
     * @brief Checks if a tutorial may start (auto-start logic)
     * @param name The tutorial name to check
     * @return true if the tutorial may start, false otherwise
     */
    bool mayStart(TutorialName name) const;

    /**
     * @brief Starts the next tutorial that is allowed to start
     */
    void startNextTutorialIfAllowed();

    bool waitForUserInput_ = false; ///< Whether to wait for user input before advancing tutorial steps
};

} // namespace QaplaWindows
