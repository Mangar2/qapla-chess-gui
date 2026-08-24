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
#include <filesystem>

namespace QaplaTest::TutorialTest {

    // Step 4: Configure Opening File
    inline void executeStep04_ConfigureOpening(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 4: Configure Opening File");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Open Opening section
        ctx->ItemOpen("**/###Opening");
        ctx->Yield();

        // The suite's own position file. This used to scan the working directory for anything
        // ending in .epd and fall back to "dummy.epd" when it found nothing -- which satisfied
        // the check two lines down and then made the tournament refuse to start eight steps
        // later, for a reason nothing in between mentioned.
        const auto openingFile = QaplaTest::testDataPath("wmtest.epd");
        IM_CHECK(std::filesystem::is_regular_file(openingFile));
        tournamentData.tournamentOpening().openings().file = openingFile;

        // Close Opening section
        ctx->ItemClose("**/###Opening");
        ctx->Yield();

        // Click Continue and advance to step 5
        clickContinueAndAdvance(ctx, 5);
        
        // Verify tutorial moved to Tournament section
        IM_CHECK_STR_EQ(QaplaWindows::TournamentWindow::highlightedSection_.c_str(), "Tournament");
    }

} // namespace QaplaTest::TutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
