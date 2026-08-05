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

#include "config-group-loader.h"
#include "tournament-config-sections.h"

#include <cli/settings-manager.h>
#include <sprt/sprt-config.h>
#include <sprt/sprt-tournament-file.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace QaplaConfiguration;

namespace {

/** @brief Reads a whole file into a string, or an empty string if it cannot be opened. */
std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

/** @brief Removes a scratch file, ignoring failures. */
struct ScopedFile {
    std::filesystem::path path;
    ~ScopedFile() {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
};

} // namespace

TEST_CASE("SPRT settings survive a save to the SPRT state file", "[config-persistence][sprt]") {
    // The engine-tester builds the state file's contents from Settings::Manager, which fills
    // in each group's schema default id. SprtTournamentFile::save() only picks up sections
    // whose id matches the file's own id (or "all"), so a mismatching default silently
    // dropped the entire [sprt] section -- engines, time control and openings were written,
    // but eloH0/eloH1/maxgames were lost.
    QaplaTester::SprtConfig config;
    config.eloH0 = 0.0F;
    config.eloH1 = 5.0F;
    config.alpha = 0.05;
    config.beta = 0.05;
    config.maxGames = 20000;
    config.model = "logistic";
    config.pentanomial = true;

    // Deliberately written without an "id" entry, so the schema default decides -- that is
    // exactly the path the engine-tester takes.
    auto section = toSprtSection(config, "test-id");
    section.eraseEntry("id");

    auto& manager = loadGroupIntoManager("sprt", {section});
    const auto configData = manager.toConfigData({"sprt"});

    SECTION("the section is filed under the id the state file is saved with") {
        const auto sections =
            configData.getSectionList("sprt", QaplaTester::SprtTournamentFile::id);
        REQUIRE(sections.has_value());
        REQUIRE_FALSE(sections->empty());
    }

    SECTION("the written file actually contains the SPRT settings") {
        const ScopedFile file{std::filesystem::temp_directory_path() /
                              "qapla-sprt-file-test.qsprt"};
        QaplaTester::SprtTournamentFile::save(file.path.string(), configData,
                                              QaplaTester::SprtTournamentFile::id);

        // Keys come back lower-cased: Settings::Manager normalises them on parse, and they
        // are matched case-insensitively on read, so the casing is not significant here.
        const std::string written = readFile(file.path);
        INFO("file contents:\n" << written);
        CHECK(written.find("[sprt]") != std::string::npos);
        CHECK(written.find("maxgames=20000") != std::string::npos);
        CHECK(written.find("eloh0=0") != std::string::npos);
        CHECK(written.find("eloh1=5") != std::string::npos);
    }
}
