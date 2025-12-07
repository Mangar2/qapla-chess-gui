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

#include "tutorial-test-helpers.h"

namespace QaplaTest::TutorialTest {

    // Step 3: Select two instances of same engine with ponder enabled on first
    inline void executeStep03_SelectEngines(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 3: Select Engines");

        // Open Engines section
        ctx->ItemOpen("**/###Engines");
        ctx->Yield();

        // Select first engine twice via + button
        ctx->ItemClick("**/available_0/###addEngine", 0, ImGuiTestOpFlags_NoError);
        ctx->ItemClick("**/available_0/###addEngine", 0, ImGuiTestOpFlags_NoError);
        ctx->Yield();

        // Open first selected engine to enable ponder
        ctx->ItemOpen("**/###0selected");
        ctx->Yield();
        
        ctx->ItemCheck("**/###Ponder");
        ctx->Yield();

        // Close the engine configuration
        ctx->ItemClose("**/###0selected");
        ctx->Yield();
        
        // Close Engines section
        ctx->ItemClose("**/###Engines");
        ctx->Yield();

        // Click Continue and advance to step 4
        clickContinueAndAdvance(ctx, 4);
        
        // Verify tutorial moved to Opening section
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "Opening");
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
