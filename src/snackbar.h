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

#include <imgui.h>

#include <string>
#include <chrono>
#include <array>
#include <deque>
#include <functional>

#include "callback-manager.h" 

namespace QaplaWindows {

/**
 * @brief Manager for displaying temporary notification messages (snackbars) in the UI
 * 
 * The SnackbarManager provides a singleton pattern for displaying toast-like notifications
 * with different severity levels. Messages are displayed at the bottom-left of the viewport
 * and automatically disappear after a configured duration.
 */
class SnackbarManager {
public:
    enum class SnackbarType {
        Note,
        Success,
        Warning,
        Error,
        Count
    };

    /**
     * @brief Configuration settings for the SnackbarManager
     */
    struct SnackbarConfig {
        uint32_t noteDurationInS = 10;
        uint32_t successDurationInS = 10;
        uint32_t warningDurationInS = 15;
        uint32_t errorDurationInS = 20;

        bool operator==(const SnackbarConfig& other) const {
            return noteDurationInS == other.noteDurationInS &&
                   successDurationInS == other.successDurationInS &&
                   warningDurationInS == other.warningDurationInS &&
                   errorDurationInS == other.errorDurationInS;
        }
    };

    /**
     * @brief Represents a single snackbar notification entry
     */
    struct SnackbarEntry {
        std::string message;
        std::string topic;
        std::chrono::steady_clock::time_point startTime;
        SnackbarType type;
        float duration;
        bool sticky;
        bool isTutorial;
    };

    /**
     * @brief Callback type for filtering snackbar entries before display
     * 
     * The callback receives the entry before it is shown. If it returns false,
     * the entry will not be displayed (but still added to history).
     * Multiple callbacks can be registered. All callbacks are always called.
     * If ANY callback returns false, the entry will not be displayed.
     */
    using FilterCallback = std::function<bool(const SnackbarEntry&)>;
    using FilterCallbackManager = Callback::ManagerBase<bool, const SnackbarEntry&>;

    /**
     * @brief Displays an error message snackbar
     * @param message The error message to display
     * @param sticky If true, the snackbar will not close automatically
     * @param topic Optional topic for categorization (default: "")
     */
    void showError(const std::string& message, bool sticky = false, const std::string& topic = "") {
        show(message, SnackbarType::Error, sticky, false, topic);
    }
    
    /**
     * @brief Displays a warning message snackbar
     * @param message The warning message to display
     * @param sticky If true, the snackbar will not close automatically
     * @param topic Optional topic for categorization (default: "")
     */
    void showWarning(const std::string& message, bool sticky = false, const std::string& topic = "") {
        show(message, SnackbarType::Warning, sticky, false, topic);
    }
    
    /**
     * @brief Displays a success message snackbar
     * @param message The success message to display
     * @param sticky If true, the snackbar will not close automatically
     * @param topic Optional topic for categorization (default: "")
     */
    void showSuccess(const std::string& message, bool sticky = false, const std::string& topic = "") {
        show(message, SnackbarType::Success, sticky, false, topic);
    }
    
    /**
     * @brief Displays an informational note snackbar
     * @param message The note message to display
     * @param sticky If true, the snackbar will not close automatically
     * @param topic Optional topic for categorization (default: "")
     */
    void showNote(const std::string& message, bool sticky = false, const std::string& topic = "") {
        show(message, SnackbarType::Note, sticky, false, topic);
    }

    /**
     * @brief Displays a snackbar with the specified message and type
     * @param message The message text to display
     * @param type The type/severity of the snackbar (Note, Success, Warning, Error)
     * @param sticky If true, the snackbar will not close automatically (default: false)
     * @param isTutorial If true, indicates this snackbar is part of a tutorial (default: false)
     * @param topic Optional topic for categorization (default: "")
     * 
     * If a snackbar with the same message and type is already displayed,
     * the display duration will be reset instead of creating a duplicate.
     * The entry is always added to history, but only displayed if no filter callback
     * returns false.
     */
    void show(const std::string& message, SnackbarType type, bool sticky = false, bool isTutorial = false, const std::string& topic = "");

    void showTutorial(const std::string& message, SnackbarType type, bool sticky = false);

    /**
     * @brief Returns the singleton instance of the SnackbarManager
     * @return Reference to the singleton SnackbarManager instance
     */
    static SnackbarManager& instance();

    /**
     * @brief Renders and displays active snackbars in the UI
     * 
     * This method should be called once per frame in the main rendering loop.
     * It handles the positioning, styling, and automatic removal of expired snackbars.
     */
    void draw();

    /**
     * @brief Gets the current snackbar configuration
     * @return Reference to the current SnackbarConfig
     */
    const SnackbarConfig& getConfig() const {
        return config_;
    }

    /**
     * @brief Gets the current snackbar configuration (mutable)
     * @return Reference to the current SnackbarConfig
     */
    SnackbarConfig& getConfig() {
        return config_;
    }

    /**
     * @brief Sets the snackbar configuration
     * @param config The new configuration to apply
     */
    void setConfig(const SnackbarConfig& config) {
        config_ = config;
    }

    /**
     * @brief Loads the snackbar configuration from the configuration data
     */
    void loadConfiguration();

    /**
     * @brief Updates the configuration data with the current snackbar settings
     */
    void updateConfiguration() const;

    /**
     * @brief Checks if a tutorial message is currently visible
     * @return true if a tutorial message is currently displayed, false otherwise
     */
    bool isTutorialMessageVisible() const;

    /**
     * @brief Registers a filter callback that is called before displaying a snackbar
     * 
     * The callback receives the entry before it is shown. If it returns false,
     * the entry will not be displayed (but still added to history).
     * Multiple callbacks can be registered. All callbacks are always called.
     * If ANY callback returns false, the entry will not be displayed.
     * 
     * @param callback The filter callback function
     * @return A unique_ptr to an UnregisterHandle for automatic cleanup
     */
    [[nodiscard]] std::unique_ptr<Callback::UnregisterHandle> registerFilterCallback(FilterCallback callback) {
        return filterCallbacks_.registerCallback(std::move(callback));
    }

    /**
     * @brief Sets a filter callback (legacy interface, replaced by registerFilterCallback)
     * @deprecated Use registerFilterCallback instead for proper RAII cleanup
     * @param callback The filter callback function, or nullptr to remove all filters
     */
    [[deprecated("Use registerFilterCallback instead")]]
    void setFilterCallback(FilterCallback callback) {
        filterCallbacks_.clear();
        if (callback) {
            filterCallbacks_.registerCallback(std::move(callback));
        }
    }

    /**
     * @brief Gets the history of snackbar entries (last 100 entries)
     * @return Const reference to the history deque
     */
    [[nodiscard]] const std::deque<SnackbarEntry>& getHistory() const {
        return history_;
    }

    /**
     * @brief Clears the snackbar history
     */
    void clearHistory() {
        history_.clear();
    }

private:
    static constexpr size_t MAX_HISTORY_SIZE = 100;

    static bool tutorialInitialized_;
    uint32_t tutorialProgress_ = 0;
    void showNextTutorialStep(bool endSticky = false) const;

    static constexpr std::array<ImVec4, static_cast<size_t>(SnackbarType::Count)> colors = {
        ImVec4(0.8f, 0.8f, 0.8f, 1.0F), // Note: Light Gray
        ImVec4(0.7f, 0.8f, 0.7f, 1.0F), // Success: Light Green
        ImVec4(0.8f, 0.8f, 0.7f, 1.0F), // Warning: Light Yellow
        ImVec4(0.8f, 0.7f, 0.7f, 1.0F)  // Error: Light Red
    };

    static constexpr std::array<const char*, static_cast<size_t>(SnackbarType::Count)> typeNames = {
        "Note",    
        "Success", 
        "Warning", 
        "Error"    
    };

    /**
     * @brief Gets the duration for a specific snackbar type based on current configuration
     * @param type The snackbar type
     * @return Duration in seconds
     */
    float getDuration(SnackbarType type) const;

    SnackbarConfig config_;
    std::deque<SnackbarEntry> snackbarStack_;
    std::deque<SnackbarEntry> history_;
    FilterCallbackManager filterCallbacks_;
    uint32_t progress = 0;
};

} // namespace QaplaWindows
