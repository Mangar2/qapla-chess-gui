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
#include <vector>
#include <functional>

namespace QaplaWindows {

 /**
  * Native file dialogs with synchronous and asynchronous variants.
  * 
  * Synchronous methods block until the dialog is closed.
  * Asynchronous methods return immediately and call the callback when done.
  * 
  * Note: All dialogs use the current GLFW context window as parent
  * to ensure proper modal behavior (dialog stays on top of the main window).
  */
class OsDialogs {
public:
    // ========================================================================
    // Type aliases for callbacks
    // ========================================================================
    
    using OpenFileCallback = std::function<void(const std::vector<std::string>& files)>;
    using SaveFileCallback = std::function<void(const std::string& file)>;
    using SelectFolderCallback = std::function<void(const std::string& folder)>;

    // ========================================================================
    // Synchronous API (blocking) - existing interface
    // ========================================================================
    /**
     * Opens a native file dialog for selecting files.
     *
     * @param multiple If true, allows selecting multiple files.
     * @param filters List of file type filters as pairs of (description, pattern).
     *                Example: { {"PGN Files", "*.pgn"}, {"All Files", "*.*"} }
     *                If empty, no file type filtering is applied.
     * @return Vector of selected file paths, empty if cancelled.
     */
    static std::vector<std::string> openFileDialog(bool multiple = false,
        const std::vector<std::pair<std::string, std::string>>& filters = {});

    /**
     * Opens a native file dialog for saving a file.
     *
     * @param filters List of file type filters as pairs of (description, pattern).
     *                Example: { {"Text Files", "*.txt"}, {"All Files", "*.*"} }
     * @param defaultPath The default file path to show in the dialog.
     * @return The selected file path, or an empty string if cancelled.
     */
    static std::string saveFileDialog(const std::vector<std::pair<std::string, std::string>>& filters, 
        const std::string& defaultPath = {});

    /**
     * Opens a native folder selection dialog.
     *
     * @param defaultPath The default folder path to show in the dialog.
     * @return The selected folder path, or an empty string if cancelled.
     */
    static std::string selectFolderDialog(const std::string& defaultPath = {});

    // ========================================================================
    // Asynchronous API (non-blocking) - new interface
    // ========================================================================

    /**
     * Opens a native file dialog for selecting files (async version).
     * Returns immediately, calls callback when dialog is closed.
     * The ImGui render loop continues while the dialog is open.
     *
     * @param callback Called with selected files (empty vector if cancelled).
     * @param multiple If true, allows selecting multiple files.
     * @param filters List of file type filters as pairs of (description, pattern).
     */
    static void openFileDialogAsync(OpenFileCallback callback,
        bool multiple = false,
        const std::vector<std::pair<std::string, std::string>>& filters = {});

    /**
     * Opens a native file dialog for saving a file (async version).
     * Returns immediately, calls callback when dialog is closed.
     *
     * @param callback Called with selected file path (empty string if cancelled).
     * @param filters List of file type filters as pairs of (description, pattern).
     * @param defaultPath The default file path to show in the dialog.
     */
    static void saveFileDialogAsync(SaveFileCallback callback,
        const std::vector<std::pair<std::string, std::string>>& filters,
        const std::string& defaultPath = {});

    /**
     * Opens a native folder selection dialog (async version).
     * Returns immediately, calls callback when dialog is closed.
     *
     * @param callback Called with selected folder path (empty string if cancelled).
     * @param defaultPath The default folder path to show in the dialog.
     */
    static void selectFolderDialogAsync(SelectFolderCallback callback,
        const std::string& defaultPath = {});

    /**
     * Gets the platform-specific configuration directory for qapla-chess-gui.
     * 
     * @return Configuration directory path:
     *         - Windows: %LOCALAPPDATA%/qapla-chess-gui
     *         - Linux/Mac: ~/.qapla-chess-gui
     *         Returns empty string on error.
     */
    static std::string getConfigDirectory();
};

} // namespace QaplaWindows