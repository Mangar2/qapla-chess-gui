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

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "imgui_te_engine.h"

namespace QaplaTest {
    /**
     * @brief Registers all engine-setup tutorial-related tests with the ImGui test engine
     * @param engine The ImGui test engine instance
     */
    void registerEngineSetupTutorialTests(ImGuiTestEngine* engine);

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
