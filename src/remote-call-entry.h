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

#include <functional>
#include <string>

/**
 * @file
 * @brief What one call from outside was, in the form anyone interested can be told about it.
 *
 * A vocabulary type, like QaplaTester::GameRecord next to StaticCallbacks::gameUpdated(): the
 * side that produces these does not know who reads them, and the side that shows them does not
 * know where they come from. Whoever wants to know registers on StaticCallbacks::remoteCall().
 */

namespace QaplaWindows {

/**
 * @brief One executed call, as it happened.
 *
 * `renderWidget` is the live control the tool built, carried here as an inert std::function --
 * built on the UI thread inside the tool handler, invoked back on the UI thread each frame it
 * stays visible. That is what lets one call answer twice: the table goes to the screen while the
 * text goes down the wire. The same shape a chatbot step uses for a control it does not own (see
 * ChatbotStepSelectEngines) -- a function, not a reference, so nothing has to know anything.
 */
struct RemoteCallEntry {
    /** @brief Wall-clock time the call was answered, "19:43:41". */
    std::string time;

    std::string toolName;

    /** @brief The raw arguments object as received. */
    std::string arguments;

    bool success = true;
    std::string content;
    std::function<void()> renderWidget;
};

} // namespace QaplaWindows
