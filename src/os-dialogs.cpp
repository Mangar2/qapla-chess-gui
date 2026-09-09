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
 * Which extensions belong to which kind of file. Every dialog in the application is opened
 * through one of these functions, so that "open a PGN" cannot mean two different things in two
 * places. How a kind is turned into a pattern and a label is in os-dialogs-file-types.cpp.
 */

#include "os-dialogs.h"


namespace QaplaWindows {

namespace {
    /** @brief The extensions of the position files a tournament or an EPD run reads. */
    const std::vector<std::string>& positionFileExtensions() {
        static const std::vector<std::string> extensions{ "epd", "fen", "raw" };
        return extensions;
    }
}

std::vector<std::string> OsDialogs::openPgnFile(bool multiple) {
    return openFileDialog({ "pgn" }, multiple);
}

std::string OsDialogs::savePgnFile(const std::string& defaultPath) {
    return saveFileDialog({ "pgn" }, defaultPath);
}

std::vector<std::string> OsDialogs::openEpdFile() {
    return openFileDialog(positionFileExtensions());
}

std::vector<std::string> OsDialogs::openOpeningsFile() {
    std::vector<std::string> extensions{ "pgn" };
    extensions.insert(extensions.end(),
        positionFileExtensions().begin(), positionFileExtensions().end());
    return openFileDialog(extensions);
}

std::string OsDialogs::saveEpdResultFile(const std::string& defaultPath) {
    return saveFileDialog({ "qepd" }, defaultPath);
}

std::vector<std::string> OsDialogs::openEpdResultFile() {
    return openFileDialog({ "qepd" });
}

std::vector<std::string> OsDialogs::openTournamentFile() {
    return openFileDialog({ "qtour" });
}

std::string OsDialogs::saveTournamentFile(const std::string& defaultPath) {
    return saveFileDialog({ "qtour" }, defaultPath);
}

std::vector<std::string> OsDialogs::openSprtFile() {
    return openFileDialog({ "qsprt" });
}

std::string OsDialogs::saveSprtFile(const std::string& defaultPath) {
    return saveFileDialog({ "qsprt" }, defaultPath);
}

std::vector<std::string> OsDialogs::openEngineFile(bool multiple) {
    return openFileDialog({}, multiple);
}

std::vector<std::string> OsDialogs::openAnyFile(bool multiple) {
    return openFileDialog({}, multiple);
}

std::string OsDialogs::saveAnyFile(const std::string& defaultPath) {
    return saveFileDialog({}, defaultPath);
}

} // namespace QaplaWindows
