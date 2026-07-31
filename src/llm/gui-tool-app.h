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

#include "gui-tool-registry.h"

namespace QaplaLlm {

/**
 * @brief Registers general, app-level tools that don't belong to any one of the
 * tournament/SPRT/EPD feature groups: close_application and open_pgn_file.
 *
 * Both act via QaplaWindows::StaticCallbacks::message() (the same pub/sub bus the classic
 * chatbot's own "Switch to Tournament/SPRT/EPD View" buttons already use, see
 * gui-tool-tournament-register.cpp's switchToTournamentView() and its siblings) rather than
 * through a singleton, since neither the main window (GLFWwindow*) nor the PGN tab
 * (ImGuiGameList, not a singleton -- only tournament/SPRT/EPD have one) is otherwise reachable
 * from src/llm/. No separate pure-logic .cpp: both handlers are thin message-bus calls with no
 * meaningful UI-independent logic to unit-test.
 */
void registerAppTools(GuiToolRegistry& registry);

} // namespace QaplaLlm
