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

#pragma once

#include "llm/remote-control-server.h"

#include <string>
#include <vector>

/**
 * @file
 * @brief The one place every command-line option of the GUI is written down.
 *
 * A switch that exists only in the parser is a switch nobody finds. So the parser and `--help`
 * read from the same table in command-line.cpp: adding an option there makes it accepted and
 * makes it appear in the help output, in one edit. The README section "Command Line Options"
 * is the third copy and the only one a human has to keep in step -- there is a test that at
 * least holds `--help` to the table (see command-line-test.cpp).
 */

namespace QaplaApp {

/**
 * @brief Everything the executable was asked for on the command line.
 */
struct CommandLineOptions {
    /**
     * @brief `--help` (or `-h`, `/?`) was given: print helpText() and exit.
     *
     * No window, no configuration load, nothing else -- someone asking what the options are is
     * not asking for a chess GUI.
     */
    bool helpRequested = false;

    /** @brief The `--remote-control` switches. Off unless explicitly asked for. */
    QaplaLlm::RemoteControlOptions remoteControl;

    /**
     * @brief What was wrong with the command line, one line each, for the caller to print.
     *
     * Collected rather than printed so the parser stays free of I/O and can be tested as the
     * pure function it is. None of these stop the GUI: a typo in an option is a reason to say
     * so, not a reason to refuse to start an application the user can drive by hand anyway.
     */
    std::vector<std::string> messages;
};

/**
 * @brief Reads argv against the option table, ignoring anything that is not an option.
 *
 * Values are written with an equals sign (`--remote-control-port=8137`); a value-taking option
 * given bare is reported in CommandLineOptions::messages rather than silently swallowing the
 * next argument.
 */
[[nodiscard]] CommandLineOptions parseCommandLine(int argc, char** argv);

/**
 * @brief The text `--help` prints: usage, then one line per option, from the same table.
 */
[[nodiscard]] std::string helpText();

} // namespace QaplaApp
