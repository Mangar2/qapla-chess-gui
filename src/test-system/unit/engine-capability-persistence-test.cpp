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

#include <engine-handling/engine-capability.h>
#include <base-elements/ini-file.h>

#include <sstream>

using namespace QaplaConfiguration;
using QaplaTester::EngineOption;
using QaplaTester::EngineOptions;

namespace {
    EngineCapability roundTrip(const EngineCapability& original) {
        std::ostringstream out;
        original.save(out);

        std::istringstream in(out.str());
        auto sections = QaplaHelpers::IniFile::load(in);
        REQUIRE(sections.size() == 1);

        return EngineCapability::createFromSection(sections[0]);
    }
}

TEST_CASE("Engine capability options survive a save/load round-trip", "[engine-capability-persistence]") {
    EngineCapability capability;
    capability.setPath("/usr/bin/some-engine");
    capability.setProtocol(QaplaTester::EngineProtocol::Uci);
    capability.setName("Some Engine");
    capability.setAuthor("Someone");

    EngineOptions options;
    options.push_back(EngineOption{
        .name = "PawnValueMg",
        .type = QaplaTester::EngineOption::Type::Spin,
        .defaultValue = "100",
        .min = 50,
        .max = 300
    });
    options.push_back(EngineOption{
        .name = "Ponder",
        .type = QaplaTester::EngineOption::Type::Check,
        .defaultValue = "false"
    });
    options.push_back(EngineOption{
        .name = "Style",
        .type = QaplaTester::EngineOption::Type::Combo,
        .defaultValue = "Normal",
        .vars = {"Solid", "Normal", "Risky"}
    });
    capability.setSupportedOptions(options);

    REQUIRE(capability.getSupportedOptions().size() == 3);

    auto reloaded = roundTrip(capability);

    // Regression test: a stray extra quote in the JSON-line option encoding used to
    // corrupt every option's "type" field, making every option unparseable on reload
    // (silently dropped), so a freshly-saved engine lost all its options as soon as
    // the app was restarted.
    REQUIRE(reloaded.getSupportedOptions().size() == 3);

    const auto& reloadedOptions = reloaded.getSupportedOptions();
    REQUIRE(reloadedOptions[0].name == "PawnValueMg");
    REQUIRE(reloadedOptions[0].type == QaplaTester::EngineOption::Type::Spin);
    REQUIRE(reloadedOptions[0].defaultValue == "100");
    REQUIRE(reloadedOptions[0].min.has_value());
    REQUIRE(*reloadedOptions[0].min == 50);
    REQUIRE(reloadedOptions[0].max.has_value());
    REQUIRE(*reloadedOptions[0].max == 300);

    REQUIRE(reloadedOptions[1].name == "Ponder");
    REQUIRE(reloadedOptions[1].type == QaplaTester::EngineOption::Type::Check);

    REQUIRE(reloadedOptions[2].name == "Style");
    REQUIRE(reloadedOptions[2].type == QaplaTester::EngineOption::Type::Combo);
    REQUIRE(reloadedOptions[2].defaultValue == "Normal");
    REQUIRE(reloadedOptions[2].vars == std::vector<std::string>{"Solid", "Normal", "Risky"});
}
