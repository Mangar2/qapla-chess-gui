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

#include "sprt-tournament-test-helpers.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "sprt-tournament-data.h"
#include "engine-worker-factory.h"
#include "imgui-engine-select.h"
#include "imgui-engine-global-settings.h"
#include "chatbot/chatbot-window.h"
#include <imgui.h>

namespace QaplaTest::SprtTournamentChatbot {

    bool waitForSprtTournamentRunning(ImGuiTestContext* ctx, float maxWaitSeconds) {
        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while (!sprtTournamentData.isRunning() && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return sprtTournamentData.isRunning();
    }

    bool waitForSprtTournamentStopped(ImGuiTestContext* ctx, float maxWaitSeconds) {
        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while ((sprtTournamentData.isRunning() || sprtTournamentData.isStarting()) && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return !sprtTournamentData.isRunning() && !sprtTournamentData.isStarting();
    }

    bool waitForSprtTournamentResults(ImGuiTestContext* ctx, float maxWaitSeconds) {
        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while (!sprtTournamentData.hasResults() && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return sprtTournamentData.hasResults();
    }

    bool itemClick(ImGuiTestContext* ctx, const char* ref) {
        if (!ctx->ItemExists(ref)) {
            ctx->LogError("Item not found: %s", ref);
            return false;
        }
        ctx->ItemClick(ref);
        return true;
    }

    bool hasEnginesAvailable() {
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        return configs.size() >= 2; // SPRT tournament needs at least 2 engines
    }

    void cleanupSprtTournamentState() {
        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        if (sprtTournamentData.isRunning() || sprtTournamentData.isStarting()) {
            sprtTournamentData.stopPool(false);
        }
        sprtTournamentData.clear();
    }

    void resetChatbotToInitialState(ImGuiTestContext* ctx) {
        ctx->LogInfo("Resetting chatbot to initial state");
        QaplaWindows::ChatBot::ChatbotWindow::instance()->reset();
        ctx->Yield();
    }

    bool navigateToSprtTournamentChatbot(ImGuiTestContext* ctx) {
        if (!itemClick(ctx, "**/Chatbot###Chatbot")) {
            return false;
        }
        ctx->Yield(10);

        if (!itemClick(ctx, "**/###SPRT Tournament")) {
            return false;
        }
        ctx->Yield(10);
        return true;
    }

    void selectFirstEngineViaUI(ImGuiTestContext* ctx) {
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        if (configs.empty()) {
            ctx->LogWarning("No engines available to select");
            return;
        }
        
        // Check if first engine is already selected
        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        auto selectedEngines = sprtTournamentData.getEngineSelect().getSelectedEngines();
        
        bool alreadySelected = false;
        for (const auto& selected : selectedEngines) {
            if (selected.config.getCmd() == configs[0].getCmd()) {
                alreadySelected = true;
                break;
            }
        }
        
        if (alreadySelected) {
            ctx->LogInfo("First engine already selected, skipping");
            return;
        }
        
        // Click the checkbox for the first engine (index 0)
        ctx->ItemClick("**/tutorial/engineSettings/$$0/##select");
        ctx->Yield(5);
    }

    void selectSecondEngineViaUI(ImGuiTestContext* ctx) {
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        if (configs.size() < 2) {
            ctx->LogWarning("Not enough engines available to select second");
            return;
        }
        
        // Check if second engine is already selected
        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        auto selectedEngines = sprtTournamentData.getEngineSelect().getSelectedEngines();
        
        bool alreadySelected = false;
        for (const auto& selected : selectedEngines) {
            if (selected.config.getCmd() == configs[1].getCmd()) {
                alreadySelected = true;
                break;
            }
        }
        
        if (alreadySelected) {
            ctx->LogInfo("Second engine already selected, skipping");
            return;
        }
        
        // Click the checkbox for the second engine (index 1)
        ctx->ItemClick("**/tutorial/engineSettings/$$1/##select");
        ctx->Yield(5);
    }

    void createIncompleteSprtTournamentState(ImGuiTestContext* ctx) {
        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        
        if (configs.size() < 2) {
            ctx->LogWarning("Not enough engines for SPRT tournament");
            return;
        }

        // Clear and setup tournament
        sprtTournamentData.clear();
        
        // Create EngineConfigurations for the first two engines
        std::vector<QaplaWindows::ImGuiEngineSelect::EngineConfiguration> engineConfigs;
        
        QaplaWindows::ImGuiEngineSelect::EngineConfiguration config1;
        config1.config = QaplaTester::EngineConfig(configs[0]);
        config1.selected = true;
        engineConfigs.push_back(config1);
        
        QaplaWindows::ImGuiEngineSelect::EngineConfiguration config2;
        config2.config = QaplaTester::EngineConfig(configs[1]);
        config2.selected = true;
        engineConfigs.push_back(config2);
        
        // Set the engine configurations
        sprtTournamentData.getEngineSelect().setEngineConfigurations(engineConfigs);

        // Set opening file
        auto& opening = sprtTournamentData.tournamentOpening();
        opening.openings().file = getTestOpeningPath();
        
        // Set PGN file
        auto& pgn = sprtTournamentData.tournamentPgn();
        pgn.pgnOptions().file = getTestPgnPath();

        // Set time control to 1+0.01 (1 second + 10ms per move)
        auto& globalSettings = sprtTournamentData.getGlobalSettings();
        QaplaWindows::ImGuiEngineGlobalSettings::TimeControlSettings tcSettings;
        tcSettings.timeControl = "1.0+0.01";
        globalSettings.setTimeControlSettings(tcSettings);

        // Set concurrency for tournament
        sprtTournamentData.setExternalConcurrency(1);
        
        // Start tournament
        sprtTournamentData.startTournament();
        
        // Wait for it to start
        waitForSprtTournamentRunning(ctx, 5.0f);
        
        // Wait for actual results (at least one game completed)
        ctx->LogInfo("Waiting for SPRT tournament results...");
        waitForSprtTournamentResults(ctx, 30.0f);
        
        // Stop tournament
        sprtTournamentData.stopPool(false);
        
        // Wait for it to stop
        waitForSprtTournamentStopped(ctx, 10.0f);
    }

} // namespace QaplaTest::SprtTournamentChatbot

#endif // IMGUI_ENABLE_TEST_ENGINE
