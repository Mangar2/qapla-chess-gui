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

    // Step 6: Configure Time Control
    inline void executeStep06_ConfigureTimeControl(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 6: Configure Time Control");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto timeControlSettings = tournamentData.globalSettings().getTimeControlSettings();

        // Open Time Control section
        ctx->ItemOpen("**/###Time Control");
        ctx->Yield();

        // Select predefined time control: 20.0+0.02
        timeControlSettings.timeControl = "20.0+0.02";
        tournamentData.globalSettings().setTimeControlSettings(timeControlSettings);
        
        // Verify time control is set
        IM_CHECK_STR_EQ(tournamentData.globalSettings().getTimeControlSettings().timeControl.c_str(), "20.0+0.02");

        // Close Time Control section
        ctx->ItemClose("**/###Time Control");
        ctx->Yield();

        // Click Continue and advance to step 7
        clickContinueAndAdvance(ctx, 7);
        
        // Verify tutorial moved to Pgn section
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "Pgn");
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
