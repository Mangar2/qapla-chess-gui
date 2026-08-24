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

#include "../../test-environment.h"
#include "tutorial-test-helpers.h"

namespace QaplaTest::TutorialTest {

    // Step 7: Set PGN Output File
    inline void executeStep07_SetPgnFile(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 7: Set PGN Output File");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Open PGN section
        ctx->ItemOpen("**/###Pgn");
        ctx->Yield();

        // Into the run's own directory: a path relative to the working directory only worked
        // when that happened to be the top of the repository, and left the file there afterwards.
        tournamentData.tournamentPgn().pgnOptions().file =
            QaplaTest::testOutputPath("tutorial-tournament.pgn");
        
        // Verify PGN file is set
        IM_CHECK(!tournamentData.tournamentPgn().pgnOptions().file.empty());

        // Close PGN section
        ctx->ItemClose("**/###Pgn");
        ctx->Yield();

        // Click Continue and advance to step 8
        clickContinueAndAdvance(ctx, 8);
        
        // Verify highlighted section cleared
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "");
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
