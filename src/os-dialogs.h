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
    
    using SelectFolderCallback = std::function<void(const std::string& folder)>;

    // ========================================================================
    // The file kinds this application knows
    // ========================================================================
    //
    // One function per kind, and nothing else: a dialog that is spelled out at the call site is
    // spelled out differently at the next call site, and then "open a PGN" means two different
    // things in the same application. It happened -- one place asked for the pattern "pgn"
    // instead of "*.pgn" and its dialog showed no files at all. So the extensions live here,
    // once, and the generic dialogs below are private.
    //
    // Every filter can be switched off in the dialog: it offers "All files" beside the kind it
    // was opened for. A dialog that only ever shows one extension is a cage, not a filter.

    /** @brief Asks for a PGN file to read. @param multiple Allow more than one file. */
    [[nodiscard]] static std::vector<std::string> openPgnFile(bool multiple = false);

    /** @brief Asks where to write a PGN file. @param defaultPath What to offer first. */
    [[nodiscard]] static std::string savePgnFile(const std::string& defaultPath = {});

    /** @brief Asks for a file of test positions (EPD, or plain FEN lines). */
    [[nodiscard]] static std::vector<std::string> openEpdFile();

    /** @brief Asks for a file of opening positions, which is a PGN or a position file. */
    [[nodiscard]] static std::vector<std::string> openOpeningsFile();

    /** @brief Asks where to write the results of an EPD run. */
    [[nodiscard]] static std::string saveEpdResultFile(const std::string& defaultPath = {});

    /** @brief Asks for a stored EPD result to read back. */
    [[nodiscard]] static std::vector<std::string> openEpdResultFile();

    /** @brief Asks for a stored tournament to read back. */
    [[nodiscard]] static std::vector<std::string> openTournamentFile();

    /** @brief Asks where to write a tournament. */
    [[nodiscard]] static std::string saveTournamentFile(const std::string& defaultPath = {});

    /** @brief Asks for a stored SPRT test to read back. */
    [[nodiscard]] static std::vector<std::string> openSprtFile();

    /** @brief Asks where to write an SPRT test. */
    [[nodiscard]] static std::string saveSprtFile(const std::string& defaultPath = {});

    /**
     * @brief Asks for engine programs to add.
     *
     * Without an extension filter: an engine is an executable, which carries an extension on
     * Windows and none anywhere else.
     *
     * @param multiple Allow more than one file.
     */
    [[nodiscard]] static std::vector<std::string> openEngineFile(bool multiple = true);

    /**
     * @brief Asks for a file of no particular kind.
     *
     * For a path a user names for something the application does not classify -- an engine's own
     * option of type "file", say. Everything the application does classify has a function of its
     * own above.
     */
    [[nodiscard]] static std::vector<std::string> openAnyFile(bool multiple = false);

    /** @brief Asks where to write a file of no particular kind. */
    [[nodiscard]] static std::string saveAnyFile(const std::string& defaultPath = {});

    /**
     * @brief The extensions of a kind, and the label a dialog shows for it.
     *
     * Exposed only so that the derivation can be tested without opening a dialog; the dialogs
     * themselves are the only callers.
     *
     * @param extensions Extensions without a dot, e.g. {"epd", "raw"}.
     * @return A label such as "EPD, RAW files (*.epd, *.raw)", or "All files (*.*)" when empty.
     */
    [[nodiscard]] static std::string describeFileTypes(const std::vector<std::string>& extensions);

    /**
     * @brief The glob pattern of one extension, e.g. "pgn" becomes "*.pgn".
     * @param extension An extension without a dot; empty gives the pattern for any file.
     */
    [[nodiscard]] static std::string fileTypePattern(const std::string& extension);

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
     *         Windows and Linux return an empty string on error; macOS forwards to
     *         QaplaHelpers::OsHelpers::getConfigDirectory() and returns its fallback path.
     */
    static std::string getConfigDirectory();

private:
    /**
     * @brief Opens a native dialog for choosing files to read.
     *
     * Private on purpose: see the file kinds above. The extensions carry no dot and no star --
     * the pattern is built here, so that no caller can spell it differently.
     *
     * @param extensions Extensions to offer, without a dot. Empty offers every file.
     * @param multiple If true, allows selecting more than one file.
     * @param allowNoFilter If true, "All files" is offered beside the extensions, so the user can
     *        reach a file whose name does not end the expected way.
     * @return The chosen paths, empty if the dialog was cancelled.
     */
    [[nodiscard]] static std::vector<std::string> openFileDialog(
        const std::vector<std::string>& extensions = {},
        bool multiple = false,
        bool allowNoFilter = true);

    /**
     * @brief Opens a native dialog for choosing where to write a file.
     *
     * @param extensions Extensions to offer, without a dot. The first one is appended to a name
     *        that was typed without one.
     * @param defaultPath The path to offer first.
     * @param allowNoFilter If true, "All files" is offered beside the extensions.
     * @return The chosen path, empty if the dialog was cancelled.
     */
    [[nodiscard]] static std::string saveFileDialog(
        const std::vector<std::string>& extensions = {},
        const std::string& defaultPath = {},
        bool allowNoFilter = true);
};

} // namespace QaplaWindows