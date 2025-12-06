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

#include "tournament-test-helpers.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "tournament-data.h"
#include "engine-worker-factory.h"
#include "tournament.h"
#include "imgui-engine-select.h"
#include <imgui.h>

namespace QaplaTest::TournamentChatbot {

    bool waitForTournamentRunning(ImGuiTestContext* ctx, float maxWaitSeconds) {
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while (!tournamentData.isRunning() && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return tournamentData.isRunning();
    }

    bool waitForTournamentStopped(ImGuiTestContext* ctx, float maxWaitSeconds) {
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        
        float waited = 0.0f;
        constexpr float sleepInterval = 0.1f;
        while ((tournamentData.isRunning() || tournamentData.isStarting()) && waited < maxWaitSeconds) {
            ctx->SleepNoSkip(sleepInterval, sleepInterval);
            waited += sleepInterval;
        }
        return !tournamentData.isRunning() && !tournamentData.isStarting();
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
        return configs.size() >= 2; // Tournament needs at least 2 engines
    }

    void cleanupTournamentState() {
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        if (tournamentData.isRunning() || tournamentData.isStarting()) {
            tournamentData.stopPool(false);
        }
        tournamentData.clear(false);
    }

    bool navigateToTournamentChatbot(ImGuiTestContext* ctx) {
        if (!itemClick(ctx, "**/###Chatbot")) {
            return false;
        }
        ctx->Yield(10);

        if (!itemClick(ctx, "**/Chatbot/###Tournament")) {
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
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto selectedEngines = tournamentData.engineSelect().getSelectedEngines();
        
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
        ctx->ItemClick("**/tournamentEngineSelect/engineSettings/$$0/##select");
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
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto selectedEngines = tournamentData.engineSelect().getSelectedEngines();
        
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
        ctx->ItemClick("**/tournamentEngineSelect/engineSettings/$$1/##select");
        ctx->Yield(5);
    }

    void createIncompleteTournamentState(ImGuiTestContext* ctx) {
        auto& tournamentData = QaplaWindows::TournamentData::instance();
        auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManager();
        auto configs = configManager.getAllConfigs();
        
        if (configs.size() < 2) {
            ctx->LogWarning("Not enough engines for tournament");
            return;
        }

        // Clear and setup tournament
        tournamentData.clear(false);
        
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
        
        // Set the engine configurations via the proper API
        tournamentData.engineSelect().setEngineConfigurations(engineConfigs);

        // Set opening file
        auto& opening = tournamentData.tournamentOpening();
        opening.openings().file = getTestOpeningPath();
        
        // Set PGN file
        auto& pgn = tournamentData.tournamentPgn();
        pgn.pgnOptions().file = getTestPgnPath();

        // Configure for minimal tournament
        tournamentData.config().rounds = 1;
        tournamentData.config().games = 2;
        tournamentData.config().repeat = 1;

        // Start tournament briefly to create scheduled tasks
        tournamentData.startTournament();
        
        // Wait for it to start
        waitForTournamentRunning(ctx, 5.0f);
        
        // Wait a bit for engine to stabilize (prevents crash from rapid start/quit)
        ctx->SleepNoSkip(0.5f, 0.1f);
        
        // Stop it immediately to create "incomplete" state
        tournamentData.stopPool(false);
        
        // Wait for it to stop
        waitForTournamentStopped(ctx, 10.0f);
    }

} // namespace QaplaTest::TournamentChatbot

#endif // IMGUI_ENABLE_TEST_ENGINE
