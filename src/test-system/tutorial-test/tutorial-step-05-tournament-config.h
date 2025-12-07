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
#include "tournament.h"

namespace QaplaTest::TutorialTest {

    // Step 5: Configure Tournament (type, rounds, games, repeat)
    inline void executeStep05_ConfigureTournament(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 5: Configure Tournament Settings");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        ctx->ItemOpen("**/###Tournament");
        ctx->Yield(5);

        // Open Tournament section
        tournamentData.config().type = "round-robin"; 
     
        // Set rounds to 2
        ctx->ItemInputValue("**/###Rounds", 2);
        
        // Set games per pairing to 2
        ctx->ItemInputValue("**/###Games per pairing", 2);
        
        // Set same opening to 2
        ctx->ItemInputValue("**/###Same opening", 2);
        
        // Verify settings
        IM_CHECK_STR_EQ(tournamentData.config().type.c_str(), "round-robin");
        IM_CHECK_EQ(tournamentData.config().rounds, 2U);
        IM_CHECK_EQ(tournamentData.config().games, 2U);
        IM_CHECK_EQ(tournamentData.config().repeat, 2U);

        // Close Tournament section
        ctx->ItemClose("**/###Tournament");
        ctx->Yield();

        // Click Continue and advance to step 6
        clickContinueAndAdvance(ctx, 6);
        
        // Verify tutorial moved to TimeControl section
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "TimeControl");
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
