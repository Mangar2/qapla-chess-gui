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

#include "command-line.h"
#include "version.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <string_view>

namespace QaplaApp {

namespace {

    /** @brief The port default, taken from the struct that owns it so the help cannot drift. */
    const std::string DEFAULT_PORT_TEXT = std::to_string(QaplaLlm::RemoteControlOptions{}.port);

    /** @brief Longest a help line may get before it is wrapped onto the description column. */
    constexpr std::size_t LINE_WIDTH = 96;

    /**
     * @brief One command-line option: how it may be written, what it means, what it sets.
     *
     * `apply` receives the whole result, not just its own field, so an option may also leave a
     * message behind -- an unreadable port says so instead of quietly becoming the default.
     */
    struct OptionSpec {
        /** @brief Accepted spellings, the first one leading in the help text. Empty slots unused. */
        std::array<std::string_view, 3> names;

        /** @brief What follows the `=`, e.g. `<port>`. Empty marks a switch that takes no value. */
        std::string_view valueName;

        std::string_view help;

        /** @brief Shown as "(default: ...)". Empty when there is nothing useful to say. */
        std::string_view defaultText;

        void (*apply)(CommandLineOptions& options, std::string_view value);
    };

    /**
     * @brief Every option the executable has. Add here, and only here.
     */
    const OptionSpec OPTIONS[] = {
        {{"--help", "-h", "/?"},
         "",
         "Print this list and exit. The GUI does not start.",
         "",
         [](CommandLineOptions& options, std::string_view) { options.helpRequested = true; }},

        {{"--config-dir"},
         "<path>",
         "Keep everything this session stores in <path> instead of the per-user configuration "
         "directory: qapla-chess-gui.ini with the engine list and the window settings, EPD "
         "results, logs, the auto-saved PGN, and the window layout. The directory is created if "
         "it is not there. An automated test run points this at a directory of its own, so it "
         "starts from a known state and cannot change the configuration you work with.",
         "the per-user configuration directory",
         [](CommandLineOptions& options, std::string_view value) {
             options.configDirectory = std::string(value);
         }},

        {{"--remote-desktop"},
         "",
         "Draw for a screen that is somewhere else: no background image, and the frame rate "
         "capped at 30. The same switch is in the Settings window; given here it applies to this "
         "session only and does not change what is stored. Use it over a remote desktop, over a "
         "forwarded display, and for automated test runs.",
         "off",
         [](CommandLineOptions& options, std::string_view) { options.remoteDesktop = true; }},

        {{"--remote-control"},
         "",
         "Serve the tools of this GUI over HTTP on 127.0.0.1, so another program can drive the "
         "running window: start tournaments, SPRT runs, EPD analysis and CLOP tuning, and read "
         "their results back. Whatever is started that way is played in the visible GUI.",
         "off",
         [](CommandLineOptions& options, std::string_view) { options.remoteControl.enabled = true; }},

        {{"--remote-control-port"},
         "<port>",
         "TCP port of the remote control, bound on 127.0.0.1 and never on another interface. "
         "0 asks the operating system for a free one, which the Remote Control panel then "
         "reports.",
         DEFAULT_PORT_TEXT,
         [](CommandLineOptions& options, std::string_view value) {
             // An unreadable port keeps the default rather than aborting startup: the GUI is
             // still perfectly usable by hand, and start() reports the port it really bound.
             try {
                 options.remoteControl.port = std::stoi(std::string(value));
             } catch (const std::exception&) {
                 options.messages.push_back("Cannot read \"" + std::string(value) +
                                            "\" as a port, keeping " + DEFAULT_PORT_TEXT + ".");
             }
         }},

        {{"--remote-control-token"},
         "<token>",
         "Shared secret every caller has to send as \"Authorization: Bearer <token>\". Without "
         "it, any program on this machine may drive the GUI.",
         "no token",
         [](CommandLineOptions& options, std::string_view value) {
             options.remoteControl.token = std::string(value);
         }},
    };

    /** @brief "--help, -h, /?" or "--remote-control-port=<port>", as the help column shows it. */
    [[nodiscard]] std::string namesText(const OptionSpec& option) {
        std::string text;
        for (auto name : option.names) {
            if (name.empty()) {
                continue;
            }
            if (!text.empty()) {
                text += ", ";
            }
            text += name;
            if (!option.valueName.empty()) {
                text += '=';
                text += option.valueName;
            }
        }
        return text;
    }

    /** @brief Splits "--flag=value" at the first `=`; without one the whole token is the name. */
    struct SplitArgument {
        std::string_view name;
        std::string_view value;
        bool hasValue = false;
    };

    [[nodiscard]] SplitArgument splitArgument(std::string_view argument) {
        auto position = argument.find('=');
        if (position == std::string_view::npos) {
            return {argument, {}, false};
        }
        return {argument.substr(0, position), argument.substr(position + 1), true};
    }

    [[nodiscard]] const OptionSpec* findOption(std::string_view name) {
        for (const auto& option : OPTIONS) {
            if (std::find(option.names.begin(), option.names.end(), name) != option.names.end()) {
                return &option;
            }
        }
        return nullptr;
    }

    /** @brief Writes text into the description column, breaking at spaces rather than mid-word. */
    void appendWrapped(std::ostringstream& out, std::string_view text, std::size_t indent) {
        const std::size_t available = LINE_WIDTH > indent + 20 ? LINE_WIDTH - indent : 40;
        bool continuation = false;
        std::string_view rest = text;
        while (!rest.empty()) {
            std::size_t take = rest.size();
            if (take > available) {
                auto breakAt = rest.rfind(' ', available);
                take = breakAt == std::string_view::npos ? available : breakAt;
            }
            if (continuation) {
                out << '\n' << std::string(indent, ' ');
            }
            out << rest.substr(0, take);
            rest.remove_prefix(take);
            while (!rest.empty() && rest.front() == ' ') {
                rest.remove_prefix(1);
            }
            continuation = true;
        }
    }

} // namespace

CommandLineOptions parseCommandLine(int argc, char** argv) {
    CommandLineOptions options;
    for (int index = 1; index < argc; ++index) {
        std::string_view argument = argv[index] != nullptr ? argv[index] : "";
        if (argument.empty()) {
            continue;
        }

        auto parts = splitArgument(argument);
        const OptionSpec* option = findOption(parts.name);
        if (option == nullptr) {
            // Only what looks like an option is worth complaining about; anything else may well
            // be meant for the operating system rather than for us.
            if (argument.front() == '-') {
                options.messages.push_back("Unknown option \"" + std::string(argument) +
                                           "\". Run with --help for the list.");
            }
            continue;
        }

        if (option->valueName.empty()) {
            if (parts.hasValue) {
                options.messages.push_back(std::string(parts.name) + " takes no value.");
                continue;
            }
            option->apply(options, {});
            continue;
        }

        if (!parts.hasValue || parts.value.empty()) {
            options.messages.push_back(std::string(parts.name) + " needs a value, written as " +
                                       std::string(parts.name) + "=" +
                                       std::string(option->valueName) + ".");
            continue;
        }
        option->apply(options, parts.value);
    }
    return options;
}

std::string helpText() {
    std::size_t widest = 0;
    for (const auto& option : OPTIONS) {
        widest = std::max(widest, namesText(option).size());
    }
    const std::size_t indent = widest + 4;

    std::ostringstream out;
    out << "Qapla Chess GUI " << VERSION
        << " - a chess GUI for engine developers and testers.\n\n"
        << "Usage: qapla-chess-gui [options]\n\n"
        << "Options:\n";
    for (const auto& option : OPTIONS) {
        auto names = namesText(option);
        out << "  " << names << std::string(indent - 2 - names.size(), ' ');
        std::string description(option.help);
        if (!option.defaultText.empty()) {
            description += " (default: " + std::string(option.defaultText) + ")";
        }
        appendWrapped(out, description, indent);
        out << '\n';
    }
    out << "\nWithout options the GUI starts as usual; everything else is configured inside it.\n"
        << "The HTTP endpoints served by --remote-control are listed in README.md, section\n"
        << "\"Command Line Options\".\n";
    return out.str();
}

} // namespace QaplaApp
