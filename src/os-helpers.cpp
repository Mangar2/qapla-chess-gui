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

#include "os-helpers.h"

#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace QaplaHelpers {

std::string OsHelpers::getConfigDirectory() {
    namespace fs = std::filesystem;

#ifdef _WIN32
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, "LOCALAPPDATA") == 0 && buf != nullptr) {
        std::string path(buf);
        free(buf);
        return path + "/qapla-chess-gui";
    }
    // Fallback if LOCALAPPDATA is not set
    return std::string(".") + "/qapla-chess-gui";
#else
    return std::string(std::getenv("HOME")) + "/.qapla-chess-gui";
#endif
}

void OsHelpers::openInShell(const std::string& path) {
#ifdef _WIN32
    // Windows: Use ShellExecuteA to open with default application
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
    // macOS: Use "open" command
    std::string command = "open \"" + path + "\"";
    std::system(command.c_str());
#else
    // Linux: Use xdg-open
    std::string command = "xdg-open \"" + path + "\"";
    std::system(command.c_str());
#endif
}

} // namespace QaplaHelpers
