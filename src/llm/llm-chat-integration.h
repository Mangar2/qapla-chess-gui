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

namespace QaplaLlm {

/**
 * @brief Wires the LLM chat feature into the running application. Call once
 * during startup, after the GUI singletons it registers tools against exist.
 *
 * Does three things:
 * 1. Registers every GUI tool group (see gui-tool-*.h) with
 *    GuiToolRegistry::instance() -- this is where a new tool group's
 *    registerXxxTools() call belongs when adding one.
 * 2. Hooks GuiToolRegistry::instance().processQueue() into
 *    QaplaWindows::StaticCallbacks::poll() for the lifetime of the process,
 *    so tool calls made from any chat get executed on the UI thread.
 * 3. Starts asynchronous LM Studio detection and registers the AI chat
 *    thread in the ChatbotWindow once the result is known (if LM Studio
 *    was found and the feature is enabled in the configuration).
 *
 * Non-blocking throughout: the detection probe runs on a worker thread and
 * is polled the same way as the tool queue.
 */
void initializeLlmChat();

} // namespace QaplaLlm
