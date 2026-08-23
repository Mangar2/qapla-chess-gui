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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include <catch2/catch_test_macros.hpp>

#include "os-helpers.h"

#include <string>

/**
 * @file
 * @brief Holds the one function that keeps a test run away from the user's own configuration.
 *
 * Every file the GUI stores between sessions is addressed through
 * QaplaHelpers::OsHelpers::getConfigDirectory(). So this is the whole of the guarantee that
 * --config-dir gives: if the override wins here, a test run cannot reach the real
 * qapla-chess-gui.ini, and it cannot silently read what an earlier session left there either.
 */

using QaplaHelpers::OsHelpers;

namespace {

/**
 * @brief Puts the override back to where the test found it.
 *
 * The override is process-wide, and the tests share a process. Without this, one test would set
 * the configuration directory for every test that runs after it.
 */
class OverrideGuard {
public:
    OverrideGuard() : previous_(OsHelpers::configDirectoryOverride()) {}
    ~OverrideGuard() { OsHelpers::setConfigDirectoryOverride(previous_); }

    OverrideGuard(const OverrideGuard&) = delete;
    OverrideGuard& operator=(const OverrideGuard&) = delete;

private:
    std::string previous_;
};

} // namespace

TEST_CASE("Without an override the per-user configuration directory is used", "[config-directory]") {
    OverrideGuard guard;
    OsHelpers::setConfigDirectoryOverride("");

    REQUIRE(OsHelpers::configDirectoryOverride().empty());

    auto directory = OsHelpers::getConfigDirectory();
    REQUIRE_FALSE(directory.empty());
    // The name the application owns, wherever the platform puts it: ~/.qapla-chess-gui or
    // %LOCALAPPDATA%\qapla-chess-gui.
    REQUIRE(directory.find("qapla-chess-gui") != std::string::npos);
}

TEST_CASE("An override takes the place of the per-user directory", "[config-directory]") {
    OverrideGuard guard;

    const std::string sandbox = "/somewhere/for/a/test/run";
    OsHelpers::setConfigDirectoryOverride(sandbox);

    REQUIRE(OsHelpers::configDirectoryOverride() == sandbox);
    REQUIRE(OsHelpers::getConfigDirectory() == sandbox);
    // Nothing of the real directory is left in the answer -- not as a prefix, not as a fallback.
    REQUIRE(OsHelpers::getConfigDirectory().find("qapla-chess-gui") == std::string::npos);
}

TEST_CASE("Clearing the override hands the directory back to the user", "[config-directory]") {
    OverrideGuard guard;

    OsHelpers::setConfigDirectoryOverride("/somewhere/for/a/test/run");
    const auto beforeClearing = OsHelpers::getConfigDirectory();

    OsHelpers::setConfigDirectoryOverride("");

    REQUIRE(OsHelpers::configDirectoryOverride().empty());
    REQUIRE(OsHelpers::getConfigDirectory() != beforeClearing);
    REQUIRE(OsHelpers::getConfigDirectory().find("qapla-chess-gui") != std::string::npos);
}
