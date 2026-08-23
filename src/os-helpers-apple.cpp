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

/**
 * =============================================================================
 * MACOS-SPECIFIC OsHelpers IMPLEMENTATIONS
 * =============================================================================
 *
 * Holds the macOS variants of QaplaHelpers::OsHelpers members that used to sit in an #ifdef
 * branch inside os-helpers.cpp. Everything defined here is the single macOS truth for its
 * function: os-helpers.cpp excludes these definitions on __APPLE__, and OsDialogs forwards to
 * them. That way a macOS change touches this file only and can never reach Windows or Linux.
 *
 * =============================================================================
 */

#ifdef __APPLE__

#include "os-helpers.h"

#include <filesystem>
#include <string>

#include <pwd.h>
#include <unistd.h>

namespace QaplaHelpers {

std::string OsHelpers::defaultConfigDirectory() {
    std::string homeDir;

    if (const auto home = getEnv("HOME")) {
        homeDir = *home;
    } else {
        // HOME can be absent when the app is started outside a login session (launchd, a stripped
        // environment). Ask the password database instead. pw_dir points into buffer, so the value
        // has to be copied out before both leave scope.
        struct passwd pwd;
        struct passwd* result = nullptr;
        char buffer[4096];

        if (getpwuid_r(getuid(), &pwd, buffer, sizeof(buffer), &result) == 0 && result != nullptr
            && pwd.pw_dir != nullptr) {
            homeDir = pwd.pw_dir;
        }
    }

    // Last resort: keep the config next to the working directory rather than returning a path
    // that starts at the filesystem root.
    if (homeDir.empty()) {
        homeDir = ".";
    }

    return (std::filesystem::path(homeDir) / ".qapla-chess-gui").string();
}

} // namespace QaplaHelpers

#endif // __APPLE__
