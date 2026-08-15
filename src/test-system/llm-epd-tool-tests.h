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

#ifdef IMGUI_ENABLE_TEST_ENGINE
struct ImGuiTestEngine;

namespace QaplaTest {

    /**
     * @brief Registers tests that drive the EPD GUI tools directly through
     * GuiToolRegistry (no LLM involved) -- mirrors registerLlmTournamentToolTests()/
     * registerLlmSprtToolTests() for the EPD tool group (see src/llm/tools/gui-tools-epd.cpp).
     * @param engine The test engine instance.
     */
    void registerLlmEpdToolTests(ImGuiTestEngine* engine);

} // namespace QaplaTest
#endif
