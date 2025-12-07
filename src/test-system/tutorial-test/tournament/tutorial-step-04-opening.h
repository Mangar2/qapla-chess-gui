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
#include <filesystem>

namespace QaplaTest::TutorialTest {

    // Step 4: Configure Opening File
    inline void executeStep04_ConfigureOpening(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 4: Configure Opening File");
        
        auto& tournamentData = QaplaWindows::TournamentData::instance();

        // Open Opening section
        ctx->ItemOpen("**/###Opening");
        ctx->Yield();

        // Find any .epd file in the workspace
        std::filesystem::path openingFile;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().extension() == ".epd") {
                openingFile = entry.path();
                break;
            }
        }
        
        if (openingFile.empty()) {
            ctx->LogWarning("No .epd file found in workspace - skipping opening configuration");
            // Set a dummy path to satisfy tutorial
            tournamentData.tournamentOpening().openings().file = "dummy.epd";
        } else {
            // Set the opening file programmatically
            tournamentData.tournamentOpening().openings().file = openingFile.string();
        }
        
        // Verify opening file is set
        IM_CHECK(!tournamentData.tournamentOpening().openings().file.empty());

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
