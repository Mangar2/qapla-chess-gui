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

#include "imgui_te_context.h"
#include <string>
#include <filesystem>

namespace QaplaTest::SprtTournamentChatbot {

    // =========================================================================
    // Test Data Paths
    // =========================================================================
    
    /**
     * @brief Gets path to test opening file (EPD)
     */
    inline std::string getTestOpeningPath() {
        auto currentPath = std::filesystem::current_path();
        auto testDataPath = currentPath / "src" / "test-system" / "test-data" / "wmtest.epd";
        return testDataPath.string();
    }

    /**
     * @brief Gets path to test PGN output file
     */
    inline std::string getTestPgnPath() {
        auto currentPath = std::filesystem::current_path();
        auto testDataPath = currentPath / "output" / "test-sprt-tournament.pgn";
        return testDataPath.string();
    }

    // =========================================================================
    // Wait Helpers - Use SleepNoSkip for Fast mode compatibility
    // =========================================================================

    /**
     * @brief Waits for SPRT tournament to reach running state
     * @return true if running within timeout, false otherwise
     */
    bool waitForSprtTournamentRunning(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f);

    /**
     * @brief Waits for SPRT tournament to fully stop
     * @return true if stopped within timeout, false otherwise
     */
    bool waitForSprtTournamentStopped(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f);

    /**
     * @brief Waits for SPRT tournament to have results
     * @return true if results available within timeout, false otherwise
     */
    bool waitForSprtTournamentResults(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f);

    // =========================================================================
    // UI Helpers
    // =========================================================================

    /**
     * @brief Safely clicks an item with existence check
     * @return false if item not found (test should use IM_CHECK on result)
     */
    bool itemClick(ImGuiTestContext* ctx, const char* ref);

    /**
     * @brief Checks if engines are available in the system
     */
    bool hasEnginesAvailable();

    /**
     * @brief Cleans up SPRT tournament state - call at start AND end of tests
     */
    void cleanupSprtTournamentState();

    /**
     * @brief Navigates to Chatbot and selects SPRT Tournament option
     * @return true if navigation successful
     */
    bool navigateToSprtTournamentChatbot(ImGuiTestContext* ctx);

    /**
     * @brief Selects the first available engine via UI checkbox
     */
    void selectFirstEngineViaUI(ImGuiTestContext* ctx);

    /**
     * @brief Selects the second available engine via UI checkbox  
     */
    void selectSecondEngineViaUI(ImGuiTestContext* ctx);

    /**
     * @brief Creates an incomplete SPRT tournament state for testing
     * Starts tournament briefly then stops it to simulate incomplete state
     */
    void createIncompleteSprtTournamentState(ImGuiTestContext* ctx);

} // namespace QaplaTest::SprtTournamentChatbot

#endif // IMGUI_ENABLE_TEST_ENGINE
