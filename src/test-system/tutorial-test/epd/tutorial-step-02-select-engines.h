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

namespace QaplaTest::EpdTutorialTest {

    // Step 2: Select at least two engines for EPD analysis
    inline void executeStep02_SelectEngines(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 2: Select Engines");

        auto& epdData = QaplaWindows::EpdData::instance();

        // Wait for EngineSelect section to be highlighted
        IM_CHECK(waitForHighlightedSection(ctx, "EngineSelect", 5.0f));
        IM_CHECK_STR_EQ(QaplaWindows::EpdWindow::highlightedSection_.c_str(), "EngineSelect");

        // Open Engines section
        ctx->ItemOpen("**/###Engines");
        ctx->Yield();

        // Select first two available engines via + button
        ctx->ItemClick("**/available_0/###addEngine", 0, ImGuiTestOpFlags_NoError);
        ctx->Yield();
        ctx->ItemClick("**/available_1/###addEngine", 0, ImGuiTestOpFlags_NoError);
        ctx->Yield();

        // Verify at least 2 engines selected
        const auto& configs = epdData.getEngineSelect().getEngineConfigurations();
        int selectedCount = 0;
        for (const auto& engineConfig : configs) {
            if (engineConfig.selected) {
                selectedCount++;
            }
        }
        IM_CHECK(selectedCount >= 2);

        // Close Engines section
        ctx->ItemClose("**/###Engines");
        ctx->Yield();

        // Click Continue and advance to step 3
        clickContinueAndAdvance(ctx, 3);
        
        // Verify tutorial moved to Configuration section
        IM_CHECK_STR_EQ(QaplaWindows::EpdWindow::highlightedSection_.c_str(), "Configuration");
        IM_CHECK(QaplaWindows::EpdWindow::configurationTutorial_.highlight);
    }

} // namespace QaplaTest::EpdTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
