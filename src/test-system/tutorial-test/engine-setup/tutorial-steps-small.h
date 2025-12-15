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

namespace QaplaTest::EngineSetupTutorialTest {

    // Step 1: Open Engines Tab
    inline void executeStep01_OpenEnginesTab(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 1: Open Engines Tab");
        
        // Click Engines tab
        ctx->ItemClick("**/QaplaTabBar/###Engines");
        ctx->Yield();

        // Wait for progress to advance to step 2
        IM_CHECK(waitForTutorialProgress(ctx, 2, 5.0f));
    }

    // Step 2a: Add FAKE engines that will fail detection
    inline void executeStep02a_AddFakeEngines(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 2a: Add Fake Engines (will fail detection)");
        ctx->Yield();

        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
        
        // Add two fake engines that don't exist
        auto config1 = QaplaTester::EngineConfig::createFromPath("FakeEngine1.exe");
        config1.setName("FakeEngine1");
        configManager.addConfig(std::move(config1));

        auto config2 = QaplaTester::EngineConfig::createFromPath("FakeEngine2.exe");
        config2.setName("FakeEngine2");
        configManager.addConfig(std::move(config2));

        ctx->Yield();

        // Wait for tutorial to advance to step 3
        IM_CHECK(waitForTutorialProgress(ctx, 3, 5.0f));
    }

    // Step 2b: Detect fake engines - MUST FAIL
    inline void executeStep02b_DetectFakeEngines(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 2b: Detect Fake Engines (must fail)");
        ctx->Yield();

        // Click Detect button
        ctx->ItemClick("**/###Engines/Detect");
        ctx->Yield();

        // Wait for detection to complete
        auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
        float waited = 0.0f;
        constexpr float sleepInterval = 0.5f;
        constexpr float maxWait = 30.0f;
        while (capabilities.isDetecting() && waited < maxWait) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        
        if (capabilities.isDetecting()) {
            IM_ERRORF("Detection did not complete within %.1f seconds", maxWait);
            return;
        }

        // Verify detection FAILED (as expected with fake engines)
        if (QaplaWindows::ImGuiEngineSelect::areAllEnginesDetected()) {
            IM_ERRORF("Detection succeeded but should have failed with fake engines");
            return;
        }

        // Verify we did NOT reach step 4 (because detection failed)
        if (QaplaWindows::EngineSetupWindow::tutorialProgress_ >= 4) {
            IM_ERRORF("Tutorial reached step 4 despite detection failure");
            return;
        }

        ctx->LogInfo("Detection correctly failed - tutorial did not advance to step 4");
    }

    // Step 2c: Remove fake engines
    inline void executeStep02c_RemoveFakeEngines(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 2c: Remove Fake Engines");
        ctx->Yield();

        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
        auto configs = configManager.getAllConfigs();
        for (const auto& cfg : configs) {
            if (cfg.getName() == "FakeEngine1" || cfg.getName() == "FakeEngine2") {
                configManager.removeConfig(cfg);
            }
        }

        ctx->Yield();

        // Verify we still did NOT reach step 4 after removal
        if (QaplaWindows::EngineSetupWindow::tutorialProgress_ >= 4) {
            IM_ERRORF("Tutorial reached step 4 after removing engines - tutorial was 'cheated'");
            return;
        }

        ctx->LogInfo("Tutorial correctly still at step 3 after engine removal");
    }

    // Step 3: Add REAL engines and detect successfully
    inline void executeStep03_AddRealEngines(ImGuiTestContext* ctx) {
        ctx->LogInfo("Step 3: Add Real Engines");
        ctx->Yield();

        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
        
        // Add two different diagnostic engines (real, working engines)
        auto config1 = QaplaTester::EngineConfig::createFromPath("src/test-system/test-data/diagnostic-engine.exe");
        config1.setName("DiagnosticEngine");
        configManager.addConfig(std::move(config1));

        auto config2 = QaplaTester::EngineConfig::createFromPath("src/test-system/test-data/diagnostic-engine-lossontime.exe");
        config2.setName("DiagnosticEngineLossOnTime");
        configManager.addConfig(std::move(config2));

        ctx->Yield();

        // Click Detect button
        ctx->ItemClick("**/###Engines/Detect");
        ctx->Yield();

        // Wait for detection to complete
        auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
        float waited = 0.0f;
        constexpr float sleepInterval = 0.5f;
        constexpr float maxWait = 30.0f;
        while (capabilities.isDetecting() && waited < maxWait) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        
        if (capabilities.isDetecting()) {
            IM_ERRORF("Detection did not complete within %.1f seconds", maxWait);
            return;
        }

        // Verify detection was successful
        if (!QaplaWindows::ImGuiEngineSelect::areAllEnginesDetected()) {
            IM_ERRORF("Engine detection failed - not all engines were successfully detected");
            return;
        }
        ctx->Yield(5);
        
        auto& tutorial = QaplaWindows::Tutorial::instance();
        const auto& entry = tutorial.getEntry(QaplaWindows::Tutorial::TutorialName::EngineSetup);
        IM_CHECK(!entry.running());
        
        // Verify we have 2 engines configured
        auto configs = configManager.getAllConfigs();
        IM_CHECK_EQ(configs.size(), static_cast<size_t>(2));
        ctx->ItemClick("**/###Close");
        ctx->LogInfo("Tutorial successfully completed with 2 detected engines");
    }

} // namespace QaplaTest::EngineSetupTutorialTest

#endif // IMGUI_ENABLE_TEST_ENGINE
