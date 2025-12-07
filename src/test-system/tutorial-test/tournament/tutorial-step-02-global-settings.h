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

    // Step 2: Configure Global Settings (Hash=64 MB, Ponder disabled)
    inline void executeStep02_ConfigureGlobalSettings(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 2: Configure Global Settings");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto& globalConfig = tournamentData.globalSettings().getGlobalConfiguration();
        
        // Wait for GlobalSettings section to be highlighted
        IM_CHECK(waitForHighlightedSection(ctx, "GlobalSettings", 5.0f));
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "GlobalSettings");
        IM_CHECK(QaplaWindows::TournamentWindow::globalSettingsTutorial_.highlight);

        // Open the GlobalSettings CollapsingHeader
        ctx->ItemOpen("**/###Global Engine Settings");
        ctx->Yield();

        // Set Hash to 64 MB via UI slider
        ctx->ItemInputValue("**/###Hash (MB)", 64);
        ctx->Yield();
        
        // Verify Hash is set
        IM_CHECK_EQ(globalConfig.hashSizeMB, 64U);

        // Disable global pondering via checkbox
        if (globalConfig.useGlobalPonder) {
            ctx->ItemUncheck("**/##usePonder");
            ctx->Yield();
        }
        
        // Verify global ponder is disabled
        IM_CHECK(!globalConfig.useGlobalPonder);

        // Close the GlobalSettings section
        ctx->ItemClose("**/###Global Engine Settings");
        ctx->Yield();

        // Click Continue and advance to step 3
        clickContinueAndAdvance(ctx, 3);
        
        // Verify tutorial moved to EngineSelect section
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "EngineSelect");
        IM_CHECK(!QaplaWindows::TournamentWindow::globalSettingsTutorial_.highlight);
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
