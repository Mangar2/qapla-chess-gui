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

#include <catch2/catch_test_macros.hpp>

#include "os-dialogs.h"

#include <string>
#include <vector>

using QaplaWindows::OsDialogs;

TEST_CASE("A file dialog builds its pattern from the extension", "[os-dialogs]") {
    // The one thing the three platform dialogs have to agree on. They used to disagree: the open
    // dialog took the second half of a filter as a pattern, the save dialog as a bare extension,
    // and the same call spelled for one showed no files at all in the other.
    CHECK(OsDialogs::fileTypePattern("pgn") == "*.pgn");
    CHECK(OsDialogs::fileTypePattern("qtour") == "*.qtour");
}

TEST_CASE("Without an extension every file matches", "[os-dialogs]") {
    // Not "*.*": an engine binary on Linux and macOS has no dot in its name.
    CHECK(OsDialogs::fileTypePattern("") == "*");
}

TEST_CASE("A file kind is spelled out with its patterns", "[os-dialogs]") {
    CHECK(OsDialogs::describeFileTypes({ "pgn" }) == "PGN files (*.pgn)");
    CHECK(OsDialogs::describeFileTypes({ "epd", "fen", "raw" })
        == "EPD, FEN, RAW files (*.epd, *.fen, *.raw)");
}

TEST_CASE("The escape from a filter is named for what it does", "[os-dialogs]") {
    CHECK(OsDialogs::describeFileTypes({}) == "All files");
}
