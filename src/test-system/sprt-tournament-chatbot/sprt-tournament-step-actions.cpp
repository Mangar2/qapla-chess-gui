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

#include "sprt-tournament-step-actions.h"
#include "sprt-tournament-test-helpers.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "sprt-tournament-data.h"
#include <imgui.h>

namespace QaplaTest::SprtTournamentChatbot {

    // =========================================================================
    // StopRunning Step
    // =========================================================================

    bool executeStopRunningStep(ImGuiTestContext* ctx, StopRunningAction action) {
        ctx->LogInfo("Executing StopRunning step...");
        ctx->Yield(5);

        switch (action) {
            case StopRunningAction::EndTournament:
                // Note: This is a bug in the source code - it uses hardcoded "Yes, end tournament"
                // instead of using getTournamentName()
                if (!itemClick(ctx, "**/###Yes, end tournament")) {
                    return false;
                }
                ctx->Yield(10);
                // Wait for tournament to actually stop
                return waitForSprtTournamentStopped(ctx, 10.0f);

            case StopRunningAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // ContinueExisting Step
    // =========================================================================

    bool executeContinueExistingStep(ImGuiTestContext* ctx, ContinueExistingAction action) {
        ctx->LogInfo("Executing ContinueExisting step...");
        ctx->Yield(5);

        switch (action) {
            case ContinueExistingAction::YesContinue:
                return itemClick(ctx, "**/###Yes, continue SPRT tournament");

            case ContinueExistingAction::No:
                return itemClick(ctx, "**/###No");

            case ContinueExistingAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // Menu Step
    // =========================================================================

    bool executeMenuStep(ImGuiTestContext* ctx, MenuAction action) {
        ctx->LogInfo("Executing Menu step...");
        ctx->Yield(5);

        switch (action) {
            case MenuAction::NewTournament:
                return itemClick(ctx, "**/###New SPRT tournament");

            case MenuAction::SaveTournament:
                // Note: This opens a file dialog which cannot be automated
                return itemClick(ctx, "**/###Save SPRT tournament");

            case MenuAction::LoadTournament:
                // Note: This opens a file dialog which cannot be automated
                return itemClick(ctx, "**/###Load SPRT tournament");

            case MenuAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // GlobalSettings Step
    // =========================================================================

    bool executeGlobalSettingsStep(ImGuiTestContext* ctx, GlobalSettingsAction action) {
        ctx->LogInfo("Executing GlobalSettings step...");
        ctx->Yield(5);

        switch (action) {
            case GlobalSettingsAction::Continue:
                return itemClick(ctx, "**/###Continue");

            case GlobalSettingsAction::MoreOptions:
                return itemClick(ctx, "**/###More Options");

            case GlobalSettingsAction::LessOptions:
                return itemClick(ctx, "**/###Less Options");

            case GlobalSettingsAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // SelectEngines Step
    // =========================================================================

    bool executeSelectEnginesStep(ImGuiTestContext* ctx, SelectEnginesAction action, bool selectEngines) {
        ctx->LogInfo("Executing SelectEngines step...");
        ctx->Yield(5);

        if (selectEngines) {
            selectFirstEngineViaUI(ctx);
            selectSecondEngineViaUI(ctx);
        }

        switch (action) {
            case SelectEnginesAction::Continue:
                return itemClick(ctx, "**/###Continue");

            case SelectEnginesAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // LoadEngine Step
    // =========================================================================

    bool executeLoadEngineStep(ImGuiTestContext* ctx, LoadEngineAction action) {
        ctx->LogInfo("Executing LoadEngine step...");
        ctx->Yield(5);

        switch (action) {
            case LoadEngineAction::AddEngines:
                // Note: Opens file dialog
                return itemClick(ctx, "**/###Add Engines");

            case LoadEngineAction::DetectContinue:
                if (ctx->ItemExists("**/###Detect & Continue")) {
                    return itemClick(ctx, "**/###Detect & Continue");
                }
                ctx->LogInfo("Detect & Continue not available (all engines detected)");
                return itemClick(ctx, "**/###Continue");

            case LoadEngineAction::SkipDetection:
                if (ctx->ItemExists("**/###Skip Detection")) {
                    return itemClick(ctx, "**/###Skip Detection");
                }
                ctx->LogInfo("Skip Detection not available");
                return itemClick(ctx, "**/###Continue");

            case LoadEngineAction::Continue:
                return itemClick(ctx, "**/###Continue");

            case LoadEngineAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // SelectGauntlet Step
    // =========================================================================

    bool executeSelectGauntletStep(ImGuiTestContext* ctx, SelectGauntletAction action) {
        ctx->LogInfo("Executing SelectGauntlet step...");
        ctx->Yield(5);

        switch (action) {
            case SelectGauntletAction::SelectFirst: {
                // Open combo and select first engine
                ctx->ItemClick("**/##GauntletEngine");
                ctx->Yield(2);
                // Click first selectable (index 0)
                ctx->ItemClick("/$FOCUSED/**/Selectable_00");
                ctx->Yield(2);
                return true;
            }

            case SelectGauntletAction::SelectSecond: {
                // Open combo and select second engine
                ctx->ItemClick("**/##GauntletEngine");
                ctx->Yield(2);
                // Click second selectable (index 1)
                ctx->ItemClick("/$FOCUSED/**/Selectable_01");
                ctx->Yield(2);
                return true;
            }

            case SelectGauntletAction::Continue:
                return itemClick(ctx, "**/###Continue");

            case SelectGauntletAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // ConfigureEngines - Combined LoadEngine + SelectGauntlet
    // =========================================================================

    bool executeConfigureEnginesStep(ImGuiTestContext* ctx) {
        ctx->LogInfo("Executing ConfigureEngines (LoadEngine + SelectGauntlet)...");
        
        // Execute LoadEngine step
        if (!executeLoadEngineStep(ctx, LoadEngineAction::Continue)) {
            return false;
        }
        ctx->Yield(10);
        
        // Execute SelectGauntlet step
        if (!executeSelectGauntletStep(ctx, SelectGauntletAction::Continue)) {
            return false;
        }
        ctx->Yield(10);
        
        return true;
    }

    // =========================================================================
    // SprtConfiguration Step
    // =========================================================================

    bool executeSprtConfigurationStep(ImGuiTestContext* ctx, SprtConfigurationAction action) {
        ctx->LogInfo("Executing SprtConfiguration step...");
        ctx->Yield(5);

        switch (action) {
            case SprtConfigurationAction::Continue:
                return itemClick(ctx, "**/###Continue");

            case SprtConfigurationAction::MoreOptions:
                return itemClick(ctx, "**/###More Options");

            case SprtConfigurationAction::LessOptions:
                return itemClick(ctx, "**/###Less Options");

            case SprtConfigurationAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // Opening Step
    // =========================================================================

    bool executeOpeningStep(ImGuiTestContext* ctx, OpeningAction action, bool setupOpeningFile) {
        ctx->LogInfo("Executing Opening step...");

        if (setupOpeningFile) {
            auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
            sprtTournamentData.tournamentOpening().openings().file = getTestOpeningPath();
        }

        ctx->Yield(10); // Let validation run

        switch (action) {
            case OpeningAction::Continue:
                return itemClick(ctx, "**/###Continue");

            case OpeningAction::MoreOptions:
                return itemClick(ctx, "**/###More Options");

            case OpeningAction::LessOptions:
                return itemClick(ctx, "**/###Less Options");

            case OpeningAction::ShowTrace:
                return itemClick(ctx, "**/###Show Trace");

            case OpeningAction::HideTrace:
                return itemClick(ctx, "**/###Hide Trace");

            case OpeningAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // PGN Step
    // =========================================================================

    bool executePgnStep(ImGuiTestContext* ctx, PgnAction action, bool setupPgnFile,
                         std::optional<bool> appendMode) {
        ctx->LogInfo("Executing PGN step...");

        auto& sprtTournamentData = QaplaWindows::SprtTournamentData::instance();
        
        if (setupPgnFile) {
            sprtTournamentData.tournamentPgn().pgnOptions().file = getTestPgnPath();
            // Default to Append mode when setting up file (ensures "Continue" button is available)
            // Can be overridden by explicit appendMode parameter
            if (!appendMode.has_value()) {
                sprtTournamentData.tournamentPgn().pgnOptions().append = true;
            }
        }
        
        if (appendMode.has_value()) {
            sprtTournamentData.tournamentPgn().pgnOptions().append = appendMode.value();
        }

        ctx->Yield(5);

        switch (action) {
            case PgnAction::Continue:
                return itemClick(ctx, "**/###Continue");

            case PgnAction::OverwriteContinue:
                if (ctx->ItemExists("**/###Overwrite & Continue")) {
                    return itemClick(ctx, "**/###Overwrite & Continue");
                }
                return itemClick(ctx, "**/###Continue");

            case PgnAction::MoreOptions:
                return itemClick(ctx, "**/###More Options");

            case PgnAction::LessOptions:
                return itemClick(ctx, "**/###Less Options");

            case PgnAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

    // =========================================================================
    // Start Step
    // =========================================================================

    bool executeStartStep(ImGuiTestContext* ctx, StartAction action) {
        ctx->LogInfo("Executing Start step...");
        ctx->Yield(5);

        switch (action) {
            case StartAction::StartTournament:
                if (!itemClick(ctx, "**/###Start SPRT tournament")) {
                    return false;
                }
                ctx->Yield(10);
                // Wait for tournament to start
                return waitForSprtTournamentRunning(ctx, 10.0f);

            case StartAction::SwitchToView:
                return itemClick(ctx, "**/###Switch to SPRT tournament View");

            case StartAction::StayInChatbot:
                return itemClick(ctx, "**/###Stay in Chatbot");

            case StartAction::Cancel:
                return itemClick(ctx, "**/###Cancel");
        }
        return false;
    }

} // namespace QaplaTest::SprtTournamentChatbot

#endif // IMGUI_ENABLE_TEST_ENGINE
