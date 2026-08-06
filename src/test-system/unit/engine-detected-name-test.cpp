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

#include <base-elements/ini-file.h>
#include <engine-handling/engine-config.h>

#include <string>

using QaplaTester::EngineConfig;

namespace {

/**
 * @brief Builds a config the way adding an engine does: only a command, no name yet.
 *
 * Goes through createFromSection() because that is the path the application uses, and it
 * applies the naming fallback (engine named after its executable) that this test depends on.
 */
EngineConfig freshlyAdded(const std::string& cmd) {
    QaplaHelpers::IniFile::Section section;
    section.name = "engine";
    section.addEntry("cmd", cmd);
    return EngineConfig::createFromSection(section);
}

/**
 * @brief Mirrors what EngineCapabilities::storeCapabilities() does with a detection result.
 *
 * Kept in the test rather than calling storeCapabilities() directly, because that needs
 * live EngineWorker instances (i.e. real engine processes). The rule under test -- adopt
 * the reported name only while the name is still the default -- is reproduced exactly.
 */
void applyDetectionResult(EngineConfig& config, const std::string& reportedName) {
    config.setReportedName(reportedName);
    if (config.hasDefaultName()) {
        config.setName(reportedName);
    }
}

} // namespace

TEST_CASE("A detected engine name survives being saved", "[engine-config]") {
    // The name reported by the engine is deliberately not written to the config file
    // (EngineConfig::toSection skips originalName), so it has to be adopted as the display
    // name at detection time -- otherwise the engine reappears under its filename after a
    // restart, even though detection had already resolved its real name.
    auto config = freshlyAdded("/engines/Qapla_032");
    REQUIRE(config.getName() == "Qapla_032");
    REQUIRE(config.hasDefaultName());

    applyDetectionResult(config, "Qapla 0.4.0-032");

    CHECK(config.getName() == "Qapla 0.4.0-032");

    const auto section = config.toSection("engine");
    const auto savedName = section.getValue("name");
    REQUIRE(savedName.has_value());
    CHECK(*savedName == "Qapla 0.4.0-032");
    // The reported name is not persisted, which is exactly why the above must hold.
    CHECK_FALSE(section.getValue("originalName").has_value());
}

TEST_CASE("A name chosen by the user is never overwritten by detection", "[engine-config]") {
    auto config = freshlyAdded("/engines/Qapla_032");
    config.setName("My tuned build");
    REQUIRE_FALSE(config.hasDefaultName());

    applyDetectionResult(config, "Qapla 0.4.0-032");

    CHECK(config.getName() == "My tuned build");
    CHECK(config.getReportedName() == "Qapla 0.4.0-032");
}

TEST_CASE("hasDefaultName distinguishes the filename placeholder from a real name",
          "[engine-config]") {
    auto config = freshlyAdded("/engines/Qapla_032");
    CHECK(config.hasDefaultName());

    config.setName("Qapla 0.4.0-032");
    CHECK_FALSE(config.hasDefaultName());

    // A config without a command has no filename to compare against.
    EngineConfig empty;
    CHECK_FALSE(empty.hasDefaultName());
}
