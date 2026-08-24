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

namespace QaplaTest::EpdTutorialTest {

    // Step 3: Configure EPD Analysis Parameters
    inline void executeStep03_ConfigureAnalysis(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 3: Configure EPD Analysis");
        
        auto& epdData = QaplaWindows::EpdData::instance();
        auto& config = epdData.config();

        // Wait for Configuration section to be highlighted
        IM_CHECK(waitForHighlightedSection(ctx, "Configuration", 5.0f));
        IM_CHECK_STR_EQ(QaplaWindows::EpdWindow::highlightedSection_.c_str(), "Configuration");
        IM_CHECK(QaplaWindows::EpdWindow::configurationTutorial_.highlight);

        // Open Configuration section
        ctx->ItemOpen("**/###Configuration");
        ctx->Yield();

        // Set Seen plies to 3
        ctx->ItemInputValue("**/###Seen plies", 3);
        ctx->Yield();
        IM_CHECK_EQ(config.seenPlies, 3U);

        // Set Max time to 10 seconds
        ctx->ItemInputValue("**/###Max time (s)", 10);
        ctx->Yield();
        IM_CHECK_EQ(config.maxTimeInS, 10ULL);

        // Set Min time to 1 second
        ctx->ItemInputValue("**/###Min time (s)", 1);
        ctx->Yield();
        IM_CHECK_EQ(config.minTimeInS, 1ULL);

        // The suite's own position file, rather than the first .epd or .raw the working
        // directory happened to contain -- with "test.epd" as a fallback that does not exist and
        // makes the analysis refuse to start several steps later.
        const auto epdFile = QaplaTest::testDataPath("wmtest.epd");
        IM_CHECK(std::filesystem::is_regular_file(epdFile));
        config.filepath = epdFile;
        
        // Verify EPD file is set
        IM_CHECK(!config.filepath.empty());

        // Close Configuration section
        ctx->ItemClose("**/###Configuration");
        ctx->Yield();

        // Click Continue and advance to step 4
        clickContinueAndAdvance(ctx, 4);
        
        // Verify Run/Stop button is highlighted
        IM_CHECK_STR_EQ(QaplaWindows::EpdWindow::highlightedButton_.c_str(), "RunStop");
    }

} // namespace QaplaTest::EpdTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
