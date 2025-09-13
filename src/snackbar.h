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
     * @brief Displays an error message snackbar
     * @param message The error message to display
     */
    void showError(const std::string& message) {
        show(message, SnackbarType::Error);
    }
    
    /**
     * @brief Displays a warning message snackbar
     * @param message The warning message to display
     */
    void showWarning(const std::string& message) {
        show(message, SnackbarType::Warning);
    }
    
    /**
     * @brief Displays a success message snackbar
     * @param message The success message to display
     */
    void showSuccess(const std::string& message) {
        show(message, SnackbarType::Success);
    }
    
    /**
     * @brief Displays an informational note snackbar
     * @param message The note message to display
     */
    void showNote(const std::string& message) {
        show(message, SnackbarType::Note);
    }

    /**
     * @brief Displays a snackbar with the specified message and type
     * @param message The message text to display
     * @param type The type/severity of the snackbar (Note, Success, Warning, Error)
     * 
     * If a snackbar with the same message and type is already displayed,
     * the display duration will be reset instead of creating a duplicate.
     */
    void show(const std::string& message, SnackbarType type);

    /**
     * @brief Returns the singleton instance of the SnackbarManager
     * @return Reference to the singleton SnackbarManager instance
     */
    static SnackbarManager& instance() {
        static SnackbarManager instance;
        return instance;
    }

    /**
     * @brief Renders and displays active snackbars in the UI
     * 
     * This method should be called once per frame in the main rendering loop.
     * It handles the positioning, styling, and automatic removal of expired snackbars.
     */
    void draw();

private:
    struct SnackbarEntry {
        std::string message;
        std::chrono::steady_clock::time_point startTime;
        SnackbarType type;
        float duration;
    };

    /**
     * @brief Draws a circular close button with an "X" mark
     * @param position The center position of the button
     * @param radius The radius of the button circle
     * @return true if the button was clicked, false otherwise
     */
    bool drawCloseButton(const ImVec2& position, float radius);

    static constexpr std::array<ImVec4, static_cast<size_t>(SnackbarType::Count)> colors = {
        ImVec4(0.8f, 0.8f, 0.8f, 1.0f), // Note: Light Gray
        ImVec4(0.7f, 0.8f, 0.7f, 1.0f), // Success: Light Green
        ImVec4(0.8f, 0.8f, 0.7f, 1.0f), // Warning: Light Yellow
        ImVec4(0.8f, 0.7f, 0.7f, 1.0f)  // Error: Light Red
    };

    static constexpr std::array<float, static_cast<size_t>(SnackbarType::Count)> durations = {
        10.0f, // Note
        10.0f, // Success
        15.0f, // Warning
        20.0f  // Error
    };

    static constexpr std::array<const char*, static_cast<size_t>(SnackbarType::Count)> typeNames = {
        "Note",    
        "Success", 
        "Warning", 
        "Error"    
    };

    std::deque<SnackbarEntry> snackbarStack_;
};