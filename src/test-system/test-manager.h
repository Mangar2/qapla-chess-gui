/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE
struct ImGuiTestEngine;
#endif

namespace QaplaTest {

    /**
     * @brief Manages the lifecycle of the Dear ImGui Test Engine.
     */
    class TestManager {
    public:
        /**
         * @brief Default constructor.
         */
        TestManager() = default;

        /**
         * @brief Destructor.
         */
        ~TestManager() = default;

        /**
         * @brief Initializes the test engine context and registers tests.
         */
        void init();

        /**
         * @brief Updates the test engine state. Should be called after glfwSwapBuffers().
         */
        void onPostSwap();

        /**
         * @brief Draws the test engine UI.
         */
        void drawDebugWindows();

        /**
         * @brief Shuts down the test engine (stops threads, unhooks).
         * Must be called BEFORE ImGui::DestroyContext().
         */
        void stop();

        /**
         * @brief Frees the test engine context.
         * Must be called AFTER ImGui::DestroyContext().
         */
        void destroy();

    private:
#ifdef IMGUI_ENABLE_TEST_ENGINE
        ImGuiTestEngine* engine_ = nullptr;
#endif
    };

} // namespace QaplaTest
