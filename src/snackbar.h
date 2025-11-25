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
     * @brief Displays an error message snackbar
     * @param message The error message to display
     * @param sticky If true, the snackbar will not close automatically
     */
    void showError(const std::string& message, bool sticky = false) {
        show(message, SnackbarType::Error, sticky);
    }
    
    /**
     * @brief Displays a warning message snackbar
     * @param message The warning message to display
     * @param sticky If true, the snackbar will not close automatically
     */
    void showWarning(const std::string& message, bool sticky = false) {
        show(message, SnackbarType::Warning, sticky);
    }
    
    /**
     * @brief Displays a success message snackbar
     * @param message The success message to display
     * @param sticky If true, the snackbar will not close automatically
     */
    void showSuccess(const std::string& message, bool sticky = false) {
        show(message, SnackbarType::Success, sticky);
    }
    
    /**
     * @brief Displays an informational note snackbar
     * @param message The note message to display
     * @param sticky If true, the snackbar will not close automatically
     */
    void showNote(const std::string& message, bool sticky = false) {
        show(message, SnackbarType::Note, sticky);
    }

    /**
     * @brief Displays a snackbar with the specified message and type
     * @param message The message text to display
     * @param type The type/severity of the snackbar (Note, Success, Warning, Error)
     * @param sticky If true, the snackbar will not close automatically (default: false)
     * @param isTutorial If true, indicates this snackbar is part of a tutorial (default: false)
     * 
     * If a snackbar with the same message and type is already displayed,
     * the display duration will be reset instead of creating a duplicate.
     */
    void show(const std::string& message, SnackbarType type, bool sticky = false, bool isTutorial = false);

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

private:
    struct SnackbarEntry {
        std::string message;
        std::chrono::steady_clock::time_point startTime;
        SnackbarType type;
        float duration;
        bool sticky;
        bool isTutorial;
    };

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
    uint32_t progress = 0;
};

} // namespace QaplaWindows
