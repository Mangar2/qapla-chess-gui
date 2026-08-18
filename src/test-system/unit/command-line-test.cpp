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

#include "command-line.h"

#include <string>
#include <vector>

using namespace QaplaApp;

namespace {

/** @brief Parses a command line written the way a user would type it. */
[[nodiscard]] CommandLineOptions parse(std::vector<const char*> arguments) {
    arguments.insert(arguments.begin(), "qapla-chess-gui");
    return parseCommandLine(static_cast<int>(arguments.size()),
        const_cast<char**>(arguments.data()));
}

} // namespace

TEST_CASE("parseCommandLine leaves everything off when nothing is asked for", "[command-line]") {
    auto options = parse({});
    REQUIRE_FALSE(options.helpRequested);
    REQUIRE_FALSE(options.remoteControl.enabled);
    REQUIRE(options.messages.empty());
}

TEST_CASE("parseCommandLine recognises every spelling of --help", "[command-line]") {
    REQUIRE(parse({"--help"}).helpRequested);
    REQUIRE(parse({"-h"}).helpRequested);
    REQUIRE(parse({"/?"}).helpRequested);
}

TEST_CASE("parseCommandLine reads the remote-control switches", "[command-line][remote-control]") {
    SECTION("a port or token alone does not switch it on") {
        auto options = parse({"--remote-control-port=9000"});
        REQUIRE_FALSE(options.remoteControl.enabled);
        REQUIRE(options.remoteControl.port == 9000);
    }

    SECTION("all three together") {
        auto options =
            parse({"--remote-control", "--remote-control-port=9001", "--remote-control-token=t"});
        REQUIRE(options.remoteControl.enabled);
        REQUIRE(options.remoteControl.port == 9001);
        REQUIRE(options.remoteControl.token == "t");
        REQUIRE(options.messages.empty());
    }

    SECTION("an unreadable port keeps the default rather than aborting startup") {
        auto options = parse({"--remote-control", "--remote-control-port=nonsense"});
        REQUIRE(options.remoteControl.enabled);
        REQUIRE(options.remoteControl.port == QaplaLlm::RemoteControlOptions{}.port);
        REQUIRE(options.messages.size() == 1);
    }
}

TEST_CASE("parseCommandLine says what it did not understand, and still starts", "[command-line]") {
    SECTION("an unknown option is reported") {
        auto options = parse({"--remote-controll"});
        REQUIRE(options.messages.size() == 1);
        REQUIRE(options.messages.front().find("--help") != std::string::npos);
        REQUIRE_FALSE(options.remoteControl.enabled);
    }

    SECTION("a value written with a space instead of an equals sign is reported") {
        // The bare name and the value arrive as two arguments; neither is silently swallowed.
        auto options = parse({"--remote-control", "--remote-control-port", "9000"});
        REQUIRE(options.remoteControl.enabled);
        REQUIRE(options.remoteControl.port == QaplaLlm::RemoteControlOptions{}.port);
        REQUIRE(options.messages.size() == 1);
        REQUIRE(options.messages.front().find("--remote-control-port=<port>") != std::string::npos);
    }

    SECTION("a value on a switch that has none is reported") {
        auto options = parse({"--remote-control=yes"});
        REQUIRE_FALSE(options.remoteControl.enabled);
        REQUIRE(options.messages.size() == 1);
    }

    SECTION("anything that is not an option is passed over in silence") {
        auto options = parse({"some-file.pgn"});
        REQUIRE(options.messages.empty());
    }
}

TEST_CASE("helpText lists every option the parser accepts", "[command-line]") {
    // The point of the shared table in command-line.cpp: a switch that works but is not in
    // --help is a switch nobody finds. This is what holds the two together.
    auto text = helpText();
    for (const auto* flag : {"--help", "-h", "/?", "--remote-control", "--remote-control-port",
             "--remote-control-token"}) {
        INFO("missing from --help: " << flag);
        REQUIRE(text.find(flag) != std::string::npos);
    }
    REQUIRE(text.find("Usage:") != std::string::npos);
    REQUIRE(text.find(std::to_string(QaplaLlm::RemoteControlOptions{}.port)) != std::string::npos);
}
