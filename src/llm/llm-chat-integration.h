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
 * @brief Starts asynchronous LM Studio detection and registers the AI chat
 * thread in the ChatbotWindow once the result is known (if LM Studio was
 * found and the feature is enabled in the configuration).
 *
 * Non-blocking: the actual probe runs on a worker thread and is polled via
 * QaplaWindows::StaticCallbacks::poll(). Call once during application startup.
 */
void startLlmChatDetection();

} // namespace QaplaLlm
