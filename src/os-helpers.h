/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#pragma once

#include <string>

namespace QaplaHelpers {

/**
 * @brief Operating system helper functions.
 * 
 * Provides cross-platform utilities for OS-specific operations like
 * getting standard directories and opening files in the default application.
 */
class OsHelpers {
public:
    OsHelpers() = delete;
    ~OsHelpers() = delete;

    /**
     * @brief Gets the configuration directory for the application.
     * 
     * Returns platform-specific config directory:
     * - Windows: %LOCALAPPDATA%/qapla-chess-gui
     * - Linux/macOS: ~/.qapla-chess-gui
     * 
     * @return Path to the configuration directory.
     */
    static std::string getConfigDirectory();

    /**
     * @brief Opens a file or URL in the default system application.
     * 
     * Uses platform-specific methods:
     * - Windows: ShellExecuteA with "open" verb
     * - macOS: "open" command
     * - Linux: "xdg-open" command
     * 
     * @param path File path or URL to open.
     */
    static void openInShell(const std::string& path);
};

} // namespace QaplaHelpers
