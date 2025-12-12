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

#include "test-system/epd-chatbot-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "epd-data.h"
#include "engine-worker-factory.h"

#include <filesystem>

namespace QaplaTest {

    namespace {
        // Helper to get the test data path
        std::string getTestEpdPath() {
            // Get the executable directory and navigate to test-data
            auto currentPath = std::filesystem::current_path();
            auto testDataPath = currentPath / "src" / "test-system" / "test-data" / "wmtest.epd";
            return testDataPath.string();
        }

        // Helper to check if engines are available
        bool hasEnginesAvailable() {
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            return !configManager.getAllConfigs().empty();
        }

        // Helper to setup EPD configuration for tests
        void setupEpdTestConfiguration() {
            auto& epdData = QaplaWindows::EpdData::instance();
            auto& config = epdData.config();
            config.filepath = getTestEpdPath();
            config.maxTimeInS = 1;  // Short time for testing
            config.minTimeInS = 1;
            config.seenPlies = 0;
            epdData.updateConfiguration();
        }

        // Helper to cleanup EPD state after tests
        void cleanupEpdState() {
            auto& epdData = QaplaWindows::EpdData::instance();
            if (epdData.isRunning() || epdData.isStarting()) {
                epdData.stopPool(false);
            }
            epdData.clear();
        }

        // Helper to select first engine via UI checkbox
        // In simple selection mode, engines are shown with checkboxes
        // Uses specific path through epdEngineSelect PushID to avoid conflicts with other engine selects
        void selectFirstEngineViaUI(ImGuiTestContext* ctx) {
            // Get the first available engine name
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            if (configs.empty()) {
                ctx->LogWarning("No engines available to select");
                return;
            }
            
            // Check if already selected in EpdData
            auto& epdData = QaplaWindows::EpdData::instance();
            auto selectedEngines = epdData.getEngineSelect().getSelectedEngines();
            if (!selectedEngines.empty()) {
                ctx->LogInfo("Engine already selected, skipping selection");
                return;
            }
            
            // Click the checkbox for the first engine (index 0)
            // Path: epdEngineSelect PushID -> engineSettings PushID -> engine index 0 -> ##select checkbox
            ctx->ItemClick("**/epdEngineSelect/engineSettings/$$0/##select");
            ctx->Yield(5);
        }

        // Helper to verify analysis is running
        // Waits up to maxWaitSeconds for isRunning() to become true
        // Uses SleepNoSkip to ensure real waiting even in Fast mode
        bool waitForAnalysisRunning(ImGuiTestContext* ctx, float maxWaitSeconds = 5.0f) {
            auto& epdData = QaplaWindows::EpdData::instance();
            
            // Wait for analysis to reach Running state (not just Starting)
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            while (!epdData.isRunning() && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return epdData.isRunning();
        }

        // Waits up to maxWaitSeconds for analysis to fully stop
        // Uses SleepNoSkip to ensure real waiting even in Fast mode
        bool waitForAnalysisStopped(ImGuiTestContext* ctx, float maxWaitSeconds = 10.0f) {
            auto& epdData = QaplaWindows::EpdData::instance();
            
            float waited = 0.0f;
            constexpr float sleepInterval = 0.1f;
            while (!epdData.isStopped() && waited < maxWaitSeconds) {
                ctx->SleepNoSkip(sleepInterval, sleepInterval);
                waited += sleepInterval;
            }
            return epdData.isStopped();
        }

    }

    void registerEpdChatbotTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test 1: Complete EPD Chatbot Flow - Start Analysis
        // Tests the happy path: no running analysis -> select engine -> 
        // configure -> start -> stay in chatbot
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Flow", "StartAnalysis");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Complete EPD Chatbot Flow - Start Analysis ===");
            
            // Precondition: Clean state
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            // Setup test configuration (set EPD file path)
            setupEpdTestConfiguration();

            // Step 1: Navigate to Chatbot tab
            ctx->LogInfo("Step 1: Navigating to Chatbot tab...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            // Step 2: Select EPD Analysis from menu
            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Step 3: Since no analysis is running, it should skip to engine selection
            // Select first available engine using checkbox
            ctx->LogInfo("Step 3: Selecting engine...");
            selectFirstEngineViaUI(ctx);

            // Step 4: Click Continue to proceed to configuration
            ctx->LogInfo("Step 4: Clicking Continue after engine selection...");
            ctx->ItemClick("**/###Continue");
            ctx->Yield(10);

            // Step 5: Configuration should be shown, click Continue 
            // (EPD file path is already set via setupEpdTestConfiguration)
            ctx->LogInfo("Step 5: Clicking Continue after configuration...");
            ctx->ItemClick("**/###Continue");
            ctx->Yield(10);

            // Step 6: Click Start Analysis
            ctx->LogInfo("Step 6: Clicking Start Analysis...");
            ctx->ItemClick("**/###Start Analysis");
            ctx->Yield(30); // Wait for analysis to start

            // Step 7: Verify analysis is running
            IM_CHECK(waitForAnalysisRunning(ctx, 5.0f));
            ctx->LogInfo("Analysis started successfully!");

            // Step 8: Click "Stay in Chatbot" to finish the flow
            ctx->LogInfo("Step 8: Clicking Stay in Chatbot...");
            ctx->ItemClick("**/###Stay in Chatbot");
            ctx->Yield(5);

            ctx->LogInfo("=== Test StartAnalysis PASSED ===");
            
            // Cleanup
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 2: Stop Running Analysis Flow
        // Tests: running analysis -> stop -> proceed to new analysis setup
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Flow", "StopRunning");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Stop Running Analysis Flow ===");
            
            // Precondition: Start an analysis first
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();
            
            // Manually start an analysis to have a running state
            auto& epdData = QaplaWindows::EpdData::instance();
            
            // We need to select an engine first
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            IM_CHECK(!configs.empty());
            
            epdData.config().engines.clear();
            epdData.config().engines.push_back(QaplaTester::EngineConfig(configs[0]));
            
            epdData.analyse();
            ctx->Sleep(0.5f); // Wait for analysis to start
            
            IM_CHECK(epdData.isRunning() || epdData.isStarting());

            // Now open the chatbot and select EPD Analysis
            ctx->LogInfo("Step 1: Navigating to Chatbot with running analysis...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Step 3: Should see "An EPD analysis is currently running" dialog
            // Click "Yes, stop analysis"
            ctx->LogInfo("Step 3: Clicking 'Yes, stop analysis'...");
            ctx->ItemClick("**/###Yes, stop analysis");
            ctx->Yield(20);

            // Verify analysis stopped
            IM_CHECK(!epdData.isRunning());
            ctx->LogInfo("Analysis stopped successfully!");

            // Step 4: Should now be at engine selection
            // Cancel to exit the flow
            ctx->LogInfo("Step 4: Clicking Cancel to exit flow...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test StopRunning PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 3: Cancel at Stop Running Dialog
        // Tests: running analysis -> cancel (keep running)
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Cancel", "AtStopDialog");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Stop Running Dialog ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();
            
            // Start an analysis
            auto& epdData = QaplaWindows::EpdData::instance();
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            IM_CHECK(!configs.empty());
            
            epdData.config().engines.clear();
            epdData.config().engines.push_back(QaplaTester::EngineConfig(configs[0]));
            
            epdData.analyse();
            ctx->Sleep(0.5f);
            
            IM_CHECK(epdData.isRunning() || epdData.isStarting());

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Click Cancel to keep analysis running
            ctx->LogInfo("Step 3: Clicking Cancel to keep analysis running...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(10);

            // Verify analysis is still running
            IM_CHECK(epdData.isRunning());
            ctx->LogInfo("Analysis continues running as expected!");

            ctx->LogInfo("=== Test AtStopDialog PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 4: Cancel at Engine Selection
        // Tests: no running analysis -> cancel at engine selection
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Cancel", "AtEngineSelection");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Engine Selection ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Cancel at engine selection without selecting any engine
            ctx->LogInfo("Step 3: Clicking Cancel at engine selection...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtEngineSelection PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test 5: Cancel at Configuration Step
        // Tests: select engine -> cancel at configuration
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Cancel", "AtConfiguration");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Configuration Step ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Select engine
            ctx->LogInfo("Step 3: Selecting engine...");
            selectFirstEngineViaUI(ctx);

            // Continue to configuration
            ctx->LogInfo("Step 4: Continue to configuration...");
            ctx->ItemClick("**/###Continue");
            ctx->Yield(10);

            // Cancel at configuration
            ctx->LogInfo("Step 5: Clicking Cancel at configuration...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test AtConfiguration PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test 6: Cancel at Start Step
        // Tests: complete flow up to start -> cancel
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Cancel", "AtStartStep");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Start Step ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();

            // Navigate through the flow
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            ctx->LogInfo("Step 3: Selecting engine...");
            selectFirstEngineViaUI(ctx);

            ctx->LogInfo("Step 4: Continue to configuration...");
            ctx->ItemClick("**/###Continue");
            ctx->Yield(10);

            ctx->LogInfo("Step 5: Continue to start...");
            ctx->ItemClick("**/###Continue");
            ctx->Yield(10);

            // Cancel at start step
            ctx->LogInfo("Step 6: Clicking Cancel at start step...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(5);

            // Verify no analysis was started
            auto& epdData = QaplaWindows::EpdData::instance();
            IM_CHECK(!epdData.isRunning() && !epdData.isStarting());
            ctx->LogInfo("No analysis started as expected!");

            ctx->LogInfo("=== Test AtStartStep PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 7: Switch to EPD View after Start
        // Tests: complete flow -> switch to EPD view
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Flow", "SwitchToEpdView");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Switch to EPD View after Start ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();

            // Navigate through the complete flow
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            ctx->LogInfo("Step 3: Selecting engine...");
            selectFirstEngineViaUI(ctx);

            ctx->LogInfo("Step 4: Continue to configuration...");
            ctx->ItemClick("**/###Continue");
            ctx->Yield(10);

            ctx->LogInfo("Step 5: Continue to start...");
            ctx->ItemClick("**/###Continue");
            ctx->Yield(10);

            ctx->LogInfo("Step 6: Start Analysis...");
            ctx->ItemClick("**/###Start Analysis");
            ctx->Yield(30);

            // Verify analysis is running before switching view
            IM_CHECK(waitForAnalysisRunning(ctx, 5.0f));
            ctx->LogInfo("Analysis started successfully!");

            // Click "Switch to EPD View"
            ctx->LogInfo("Step 7: Clicking Switch to EPD View...");
            ctx->ItemClick("**/###Switch to EPD View");
            ctx->Yield(10);

            ctx->LogInfo("=== Test SwitchToEpdView PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 8: Continue Existing Analysis - Yes
        // Tests: incomplete analysis exists -> continue it
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Continue", "ExistingYes");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing Analysis - Yes ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();
            
            // Start an analysis and then stop it to create incomplete state
            auto& epdData = QaplaWindows::EpdData::instance();
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            IM_CHECK(!configs.empty());
            
            epdData.config().engines.clear();
            epdData.config().engines.push_back(QaplaTester::EngineConfig(configs[0]));
            
            epdData.analyse();
            
            // Wait for analysis to actually start before stopping
            IM_CHECK(waitForAnalysisRunning(ctx, 5.0f));
            
            // Wait a bit to let engine receive commands (prevents engine crash from rapid start/quit)
            ctx->SleepNoSkip(0.5f, 0.1f);
            
            // Stop the analysis (not graceful) to create incomplete state
            epdData.stopPool(false);
            IM_CHECK(waitForAnalysisStopped(ctx, 5.0f));
            
            // Verify we have incomplete analysis - this is a precondition for the test
            IM_CHECK_GT(epdData.totalTests, (size_t)0);
            IM_CHECK_GT(epdData.remainingTests, (size_t)0);

            ctx->LogInfo("Created incomplete analysis state (total=%zu, remaining=%zu)", 
                         epdData.totalTests, epdData.remainingTests);

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Should see the "continue existing" dialog
            ctx->LogInfo("Step 3: Clicking 'Yes, continue analysis'...");
            IM_CHECK(ctx->ItemExists("**/###Yes, continue analysis"));
            ctx->ItemClick("**/###Yes, continue analysis");
            ctx->Yield(10);

            // Should jump to start step, click Start Analysis
            ctx->LogInfo("Step 4: Clicking 'Start Analysis'...");
            IM_CHECK(ctx->ItemExists("**/###Start Analysis"));
            ctx->ItemClick("**/###Start Analysis");
            ctx->Yield(30);

            // Verify analysis is running
            IM_CHECK(waitForAnalysisRunning(ctx, 5.0f));
            ctx->LogInfo("Analysis continued successfully!");

            // Stay in chatbot
            ctx->LogInfo("Step 5: Clicking 'Stay in Chatbot'...");
            IM_CHECK(ctx->ItemExists("**/###Stay in Chatbot"));
            ctx->ItemClick("**/###Stay in Chatbot");
            ctx->Yield(5);

            ctx->LogInfo("=== Test ExistingYes PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 9: Continue Existing Analysis - No (New Setup)
        // Tests: incomplete analysis exists -> start fresh
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Continue", "ExistingNo");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing Analysis - No (New Setup) ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();
            
            // Create incomplete analysis state
            auto& epdData = QaplaWindows::EpdData::instance();
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            IM_CHECK(!configs.empty());
            
            epdData.config().engines.clear();
            epdData.config().engines.push_back(QaplaTester::EngineConfig(configs[0]));
            
            epdData.analyse();
            
            // Wait for analysis to actually start before stopping
            IM_CHECK(waitForAnalysisRunning(ctx, 5.0f));
            
            // Wait a bit to let engine receive commands (prevents engine crash from rapid start/quit)
            ctx->SleepNoSkip(0.5f, 0.1f);
            
            epdData.stopPool(false);
            IM_CHECK(waitForAnalysisStopped(ctx, 10.0f));
            
            // Verify we have incomplete analysis - precondition
            IM_CHECK_GT(epdData.totalTests, (size_t)0);
            IM_CHECK_GT(epdData.remainingTests, (size_t)0);

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Click "No" to start fresh setup
            ctx->LogInfo("Step 3: Clicking 'No' to start fresh...");
            ctx->ItemClick("**/###No");
            ctx->Yield(10);

            // Should now be at engine selection - verify by clicking Cancel
            ctx->LogInfo("Step 4: At engine selection, clicking Cancel...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test ExistingNo PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 10: Continue Existing Analysis - Cancel
        // Tests: incomplete analysis exists -> cancel entire flow
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Continue", "ExistingCancel");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Continue Existing Analysis - Cancel ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            setupEpdTestConfiguration();
            
            // Create incomplete analysis state
            auto& epdData = QaplaWindows::EpdData::instance();
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            IM_CHECK(!configs.empty());
            
            epdData.config().engines.clear();
            epdData.config().engines.push_back(QaplaTester::EngineConfig(configs[0]));
            
            epdData.analyse();
            
            // Wait for analysis to actually start before stopping
            IM_CHECK(waitForAnalysisRunning(ctx, 5.0f));
            
            // Wait a bit to let engine receive commands (prevents engine crash from rapid start/quit)
            ctx->SleepNoSkip(0.5f, 0.1f);
            
            epdData.stopPool(false);
            IM_CHECK(waitForAnalysisStopped(ctx, 10.0f));
            
            // Verify we have incomplete analysis - precondition
            IM_CHECK_GT(epdData.totalTests, (size_t)0);
            IM_CHECK_GT(epdData.remainingTests, (size_t)0);

            size_t remainingBefore = epdData.remainingTests;

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Click "Cancel" to exit entire flow
            ctx->LogInfo("Step 3: Clicking 'Cancel' to exit flow...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(5);

            // Verify incomplete analysis state is preserved
            IM_CHECK_EQ(epdData.remainingTests, remainingBefore);
            ctx->LogInfo("Incomplete analysis state preserved as expected!");

            ctx->LogInfo("=== Test ExistingCancel PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 11: No Continue Dialog When Analysis Complete
        // Tests: completed analysis -> should skip continue dialog
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD/Chatbot/Continue", "NoDialogWhenComplete");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: No Continue Dialog When Analysis Complete ===");
            
            cleanupEpdState();
            
            IM_CHECK(hasEnginesAvailable());

            // Note: We can't easily create a "completed" state without running
            // the full analysis, so we just verify that with no incomplete 
            // analysis, we go straight to engine selection
            
            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/###EPD Analysis");
            ctx->Yield(10);

            // Should be at engine selection (no continue dialog shown)
            // Verify by looking for Cancel button (present in engine selection)
            ctx->LogInfo("Step 3: Verifying we're at engine selection (no continue dialog)...");
            ctx->ItemClick("**/###Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test NoDialogWhenComplete PASSED ===");
        };

    }

} // namespace QaplaTest
#endif
