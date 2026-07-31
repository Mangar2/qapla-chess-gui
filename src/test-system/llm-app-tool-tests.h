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
     * @brief Registers tests that drive the open_pgn_file GUI tool directly through
     * GuiToolRegistry (no LLM involved) -- see gui-tool-app-register.cpp.
     *
     * close_application is deliberately NOT covered here: actually invoking it would quit the
     * test binary itself (mid-suite, before QAPLA_TEST_SUMMARY prints), and source="dialog"
     * for open_pgn_file is likewise untested since it blocks on a native OS dialog -- same
     * reasoning as open_add_engine_dialog having no automated coverage of its own dialog path.
     * @param engine The test engine instance.
     */
    void registerLlmAppToolTests(ImGuiTestEngine* engine);

} // namespace QaplaTest
#endif
