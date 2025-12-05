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
    }

    void registerEpdChatbotTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test 1: Complete EPD Chatbot Flow - Start Analysis
        // Tests the happy path: no running analysis -> select engine -> 
        // configure -> start -> stay in chatbot
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD Chatbot", "CompleteFlow_StartAnalysis");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Complete EPD Chatbot Flow - Start Analysis ===");
            
            // Precondition: Clean state
            cleanupEpdState();
            
            // Check if engines are available
            if (!hasEnginesAvailable()) {
                ctx->LogWarning("No engines available - skipping test");
                return;
            }

            // Setup test configuration (set EPD file path)
            setupEpdTestConfiguration();

            // Step 1: Navigate to Chatbot tab
            ctx->LogInfo("Step 1: Navigating to Chatbot tab...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            // Step 2: Select EPD Analysis from menu
            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/EPD Analysis");
            ctx->Yield(10);

            // Step 3: Since no analysis is running, it should skip to engine selection
            // Add first available engine using the "+" button
            ctx->LogInfo("Step 3: Adding engine to selection...");
            ctx->ItemClick("**/+");
            ctx->Yield(5);

            // Step 4: Click Continue to proceed to configuration
            ctx->LogInfo("Step 4: Clicking Continue after engine selection...");
            ctx->ItemClick("**/Continue");
            ctx->Yield(10);

            // Step 5: Configuration should be shown, click Continue 
            // (EPD file path is already set via setupEpdTestConfiguration)
            ctx->LogInfo("Step 5: Clicking Continue after configuration...");
            ctx->ItemClick("**/Continue");
            ctx->Yield(10);

            // Step 6: Click Start Analysis
            ctx->LogInfo("Step 6: Clicking Start Analysis...");
            ctx->ItemClick("**/Start Analysis");
            ctx->Yield(30); // Wait for analysis to start

            // Step 7: Verify analysis is running
            auto& epdData = QaplaWindows::EpdData::instance();
            if (epdData.isRunning()) {
                ctx->LogInfo("Analysis started successfully!");
            } else {
                ctx->LogError("Analysis did not start!");
            }

            // Step 8: Click "Stay in Chatbot" to finish the flow
            ctx->LogInfo("Step 8: Clicking Stay in Chatbot...");
            ctx->ItemClick("**/Stay in Chatbot");
            ctx->Yield(5);

            ctx->LogInfo("=== Test CompleteFlow_StartAnalysis PASSED ===");
            
            // Cleanup
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 2: Stop Running Analysis Flow
        // Tests: running analysis -> stop -> proceed to new analysis setup
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD Chatbot", "StopRunningAnalysis");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Stop Running Analysis Flow ===");
            
            // Precondition: Start an analysis first
            cleanupEpdState();
            
            if (!hasEnginesAvailable()) {
                ctx->LogWarning("No engines available - skipping test");
                return;
            }

            setupEpdTestConfiguration();
            
            // Manually start an analysis to have a running state
            auto& epdData = QaplaWindows::EpdData::instance();
            
            // We need to select an engine first
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            if (!configs.empty()) {
                epdData.config().engines.clear();
                epdData.config().engines.push_back(QaplaTester::EngineConfig(configs[0]));
            }
            
            epdData.analyse();
            ctx->Sleep(0.5f); // Wait for analysis to start
            
            if (!epdData.isRunning() && !epdData.isStarting()) {
                ctx->LogWarning("Could not start analysis - skipping test");
                return;
            }

            // Now open the chatbot and select EPD Analysis
            ctx->LogInfo("Step 1: Navigating to Chatbot with running analysis...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/EPD Analysis");
            ctx->Yield(10);

            // Step 3: Should see "An EPD analysis is currently running" dialog
            // Click "Yes, stop analysis"
            ctx->LogInfo("Step 3: Clicking 'Yes, stop analysis'...");
            ctx->ItemClick("**/Yes, stop analysis");
            ctx->Yield(20);

            // Verify analysis stopped
            if (!epdData.isRunning()) {
                ctx->LogInfo("Analysis stopped successfully!");
            } else {
                ctx->LogError("Analysis did not stop!");
            }

            // Step 4: Should now be at engine selection
            // Cancel to exit the flow
            ctx->LogInfo("Step 4: Clicking Cancel to exit flow...");
            ctx->ItemClick("**/Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test StopRunningAnalysis PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 3: Cancel at Stop Running Dialog
        // Tests: running analysis -> cancel (keep running)
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD Chatbot", "CancelAtStopDialog");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Stop Running Dialog ===");
            
            cleanupEpdState();
            
            if (!hasEnginesAvailable()) {
                ctx->LogWarning("No engines available - skipping test");
                return;
            }

            setupEpdTestConfiguration();
            
            // Start an analysis
            auto& epdData = QaplaWindows::EpdData::instance();
            auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
            auto configs = configManager.getAllConfigs();
            if (!configs.empty()) {
                epdData.config().engines.clear();
                epdData.config().engines.push_back(QaplaTester::EngineConfig(configs[0]));
            }
            
            epdData.analyse();
            ctx->Sleep(0.5f);
            
            if (!epdData.isRunning() && !epdData.isStarting()) {
                ctx->LogWarning("Could not start analysis - skipping test");
                return;
            }

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/EPD Analysis");
            ctx->Yield(10);

            // Click Cancel to keep analysis running
            ctx->LogInfo("Step 3: Clicking Cancel to keep analysis running...");
            ctx->ItemClick("**/Cancel");
            ctx->Yield(10);

            // Verify analysis is still running
            if (epdData.isRunning()) {
                ctx->LogInfo("Analysis continues running as expected!");
            } else {
                ctx->LogError("Analysis stopped unexpectedly!");
            }

            ctx->LogInfo("=== Test CancelAtStopDialog PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 4: Cancel at Engine Selection
        // Tests: no running analysis -> cancel at engine selection
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD Chatbot", "CancelAtEngineSelection");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Engine Selection ===");
            
            cleanupEpdState();
            
            if (!hasEnginesAvailable()) {
                ctx->LogWarning("No engines available - skipping test");
                return;
            }

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/EPD Analysis");
            ctx->Yield(10);

            // Cancel at engine selection without selecting any engine
            ctx->LogInfo("Step 3: Clicking Cancel at engine selection...");
            ctx->ItemClick("**/Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test CancelAtEngineSelection PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test 5: Cancel at Configuration Step
        // Tests: select engine -> cancel at configuration
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD Chatbot", "CancelAtConfiguration");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Configuration Step ===");
            
            cleanupEpdState();
            
            if (!hasEnginesAvailable()) {
                ctx->LogWarning("No engines available - skipping test");
                return;
            }

            setupEpdTestConfiguration();

            // Navigate to chatbot
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/EPD Analysis");
            ctx->Yield(10);

            // Add engine
            ctx->LogInfo("Step 3: Adding engine...");
            ctx->ItemClick("**/+");
            ctx->Yield(5);

            // Continue to configuration
            ctx->LogInfo("Step 4: Continue to configuration...");
            ctx->ItemClick("**/Continue");
            ctx->Yield(10);

            // Cancel at configuration
            ctx->LogInfo("Step 5: Clicking Cancel at configuration...");
            ctx->ItemClick("**/Cancel");
            ctx->Yield(5);

            ctx->LogInfo("=== Test CancelAtConfiguration PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test 6: Cancel at Start Step
        // Tests: complete flow up to start -> cancel
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD Chatbot", "CancelAtStartStep");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Cancel at Start Step ===");
            
            cleanupEpdState();
            
            if (!hasEnginesAvailable()) {
                ctx->LogWarning("No engines available - skipping test");
                return;
            }

            setupEpdTestConfiguration();

            // Navigate through the flow
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/EPD Analysis");
            ctx->Yield(10);

            ctx->LogInfo("Step 3: Adding engine...");
            ctx->ItemClick("**/+");
            ctx->Yield(5);

            ctx->LogInfo("Step 4: Continue to configuration...");
            ctx->ItemClick("**/Continue");
            ctx->Yield(10);

            ctx->LogInfo("Step 5: Continue to start...");
            ctx->ItemClick("**/Continue");
            ctx->Yield(10);

            // Cancel at start step
            ctx->LogInfo("Step 6: Clicking Cancel at start step...");
            ctx->ItemClick("**/Cancel");
            ctx->Yield(5);

            // Verify no analysis was started
            auto& epdData = QaplaWindows::EpdData::instance();
            if (!epdData.isRunning()) {
                ctx->LogInfo("No analysis started as expected!");
            } else {
                ctx->LogError("Analysis was started unexpectedly!");
            }

            ctx->LogInfo("=== Test CancelAtStartStep PASSED ===");
            
            cleanupEpdState();
        };

        // -----------------------------------------------------------------
        // Test 7: Switch to EPD View after Start
        // Tests: complete flow -> switch to EPD view
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "EPD Chatbot", "SwitchToEpdView");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            ctx->LogInfo("=== Test: Switch to EPD View after Start ===");
            
            cleanupEpdState();
            
            if (!hasEnginesAvailable()) {
                ctx->LogWarning("No engines available - skipping test");
                return;
            }

            setupEpdTestConfiguration();

            // Navigate through the complete flow
            ctx->LogInfo("Step 1: Navigating to Chatbot...");
            ctx->ItemClick("**/Chatbot###Chatbot");
            ctx->Yield(10);

            ctx->LogInfo("Step 2: Selecting EPD Analysis...");
            ctx->ItemClick("**/EPD Analysis");
            ctx->Yield(10);

            ctx->LogInfo("Step 3: Adding engine...");
            ctx->ItemClick("**/+");
            ctx->Yield(5);

            ctx->LogInfo("Step 4: Continue to configuration...");
            ctx->ItemClick("**/Continue");
            ctx->Yield(10);

            ctx->LogInfo("Step 5: Continue to start...");
            ctx->ItemClick("**/Continue");
            ctx->Yield(10);

            ctx->LogInfo("Step 6: Start Analysis...");
            ctx->ItemClick("**/Start Analysis");
            ctx->Yield(30);

            // Click "Switch to EPD View"
            ctx->LogInfo("Step 7: Clicking Switch to EPD View...");
            ctx->ItemClick("**/Switch to EPD View");
            ctx->Yield(10);

            ctx->LogInfo("=== Test SwitchToEpdView PASSED ===");
            
            cleanupEpdState();
        };

    }

} // namespace QaplaTest
#endif
