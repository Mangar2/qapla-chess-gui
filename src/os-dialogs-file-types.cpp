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
 * Extensions in, pattern and label out. The one thing the three platform dialogs must agree on,
 * kept apart from all three so that it can be checked without opening a window -- the mismatch
 * this exists to prevent (one dialog filtering on "pgn", the other on "*.pgn") was invisible
 * until a user found an empty file dialog.
 */

#include "os-dialogs.h"

#include <algorithm>
#include <cctype>

namespace QaplaWindows {

namespace {
    std::string toUpper(const std::string& text) {
        std::string upper = text;
        std::ranges::transform(upper, upper.begin(),
            [](unsigned char letter) { return static_cast<char>(std::toupper(letter)); });
        return upper;
    }
}

std::string OsDialogs::fileTypePattern(const std::string& extension) {
    // "*" rather than "*.*" for the unrestricted case: a file without a dot in its name -- an
    // engine binary on Linux and macOS -- does not match "*.*".
    return extension.empty() ? "*" : "*." + extension;
}

std::string OsDialogs::describeFileTypes(const std::vector<std::string>& extensions) {
    if (extensions.empty()) {
        return "All files";
    }
    std::string kinds;
    std::string patterns;
    for (const auto& extension : extensions) {
        if (!kinds.empty()) {
            kinds += ", ";
            patterns += ", ";
        }
        kinds += toUpper(extension);
        patterns += fileTypePattern(extension);
    }
    return kinds + " files (" + patterns + ")";
}

} // namespace QaplaWindows
