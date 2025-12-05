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

#include "tournament-step-actions.h"
#include "tournament-test-helpers.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "tournament-data.h"
#include <imgui.h>

namespace QaplaTest::TournamentChatbot {

    // =========================================================================
    // StopRunning Step
    // =========================================================================

    bool executeStopRunningStep(ImGuiTestContext* ctx, StopRunningAction action) {
        ctx->LogInfo("Executing StopRunning step...");
        ctx->Yield(5);

        switch (action) {
            case StopRunningAction::EndTournament:
                if (!safeItemClick(ctx, "**/Yes, end tournament")) {
                    return false;
                }
                ctx->Yield(10);
                // Wait for tournament to actually stop
                return waitForTournamentStopped(ctx, 10.0f);

            case StopRunningAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
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
                return safeItemClick(ctx, "**/Yes, continue tournament");

            case ContinueExistingAction::No:
                return safeItemClick(ctx, "**/No");

            case ContinueExistingAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
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
                return safeItemClick(ctx, "**/New tournament");

            case MenuAction::SaveTournament:
                // Note: This opens a file dialog which cannot be automated
                return safeItemClick(ctx, "**/Save tournament");

            case MenuAction::LoadTournament:
                // Note: This opens a file dialog which cannot be automated
                return safeItemClick(ctx, "**/Load tournament");

            case MenuAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
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
                return safeItemClick(ctx, "**/Continue");

            case GlobalSettingsAction::MoreOptions:
                return safeItemClick(ctx, "**/More Options");

            case GlobalSettingsAction::LessOptions:
                return safeItemClick(ctx, "**/Less Options");

            case GlobalSettingsAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
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
                return safeItemClick(ctx, "**/Continue");

            case SelectEnginesAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
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
                return safeItemClick(ctx, "**/Add Engines");

            case LoadEngineAction::DetectContinue:
                if (ctx->ItemExists("**/Detect & Continue")) {
                    return safeItemClick(ctx, "**/Detect & Continue");
                }
                ctx->LogInfo("Detect & Continue not available (all engines detected)");
                return safeItemClick(ctx, "**/Continue");

            case LoadEngineAction::SkipDetection:
                if (ctx->ItemExists("**/Skip Detection")) {
                    return safeItemClick(ctx, "**/Skip Detection");
                }
                ctx->LogInfo("Skip Detection not available");
                return safeItemClick(ctx, "**/Continue");

            case LoadEngineAction::Continue:
                return safeItemClick(ctx, "**/Continue");

            case LoadEngineAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
        }
        return false;
    }

    // =========================================================================
    // Configuration Step
    // =========================================================================

    bool executeConfigurationStep(ImGuiTestContext* ctx, ConfigurationAction action) {
        ctx->LogInfo("Executing Configuration step...");
        ctx->Yield(5);

        switch (action) {
            case ConfigurationAction::Continue:
                return safeItemClick(ctx, "**/Continue");

            case ConfigurationAction::MoreOptions:
                return safeItemClick(ctx, "**/More Options");

            case ConfigurationAction::LessOptions:
                return safeItemClick(ctx, "**/Less Options");

            case ConfigurationAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
        }
        return false;
    }

    // =========================================================================
    // Opening Step
    // =========================================================================

    bool executeOpeningStep(ImGuiTestContext* ctx, OpeningAction action, bool setupOpeningFile) {
        ctx->LogInfo("Executing Opening step...");

        if (setupOpeningFile) {
            auto& tournamentData = QaplaWindows::TournamentData::instance();
            tournamentData.tournamentOpening().openings().file = getTestOpeningPath();
        }

        ctx->Yield(10); // Let validation run

        switch (action) {
            case OpeningAction::Continue:
                return safeItemClick(ctx, "**/Continue");

            case OpeningAction::MoreOptions:
                return safeItemClick(ctx, "**/More Options");

            case OpeningAction::LessOptions:
                return safeItemClick(ctx, "**/Less Options");

            case OpeningAction::ShowTrace:
                return safeItemClick(ctx, "**/Show Trace");

            case OpeningAction::HideTrace:
                return safeItemClick(ctx, "**/Hide Trace");

            case OpeningAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
        }
        return false;
    }

    // =========================================================================
    // PGN Step
    // =========================================================================

    bool executePgnStep(ImGuiTestContext* ctx, PgnAction action, bool setupPgnFile,
                         std::optional<bool> appendMode) {
        ctx->LogInfo("Executing PGN step...");

        auto& tournamentData = QaplaWindows::TournamentData::instance();
        
        if (setupPgnFile) {
            tournamentData.tournamentPgn().pgnOptions().file = getTestPgnPath();
        }
        
        if (appendMode.has_value()) {
            tournamentData.tournamentPgn().pgnOptions().append = appendMode.value();
        }

        ctx->Yield(5);

        switch (action) {
            case PgnAction::Continue:
                return safeItemClick(ctx, "**/Continue");

            case PgnAction::OverwriteContinue:
                if (ctx->ItemExists("**/Overwrite & Continue")) {
                    return safeItemClick(ctx, "**/Overwrite & Continue");
                }
                return safeItemClick(ctx, "**/Continue");

            case PgnAction::MoreOptions:
                return safeItemClick(ctx, "**/More Options");

            case PgnAction::LessOptions:
                return safeItemClick(ctx, "**/Less Options");

            case PgnAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
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
                if (!safeItemClick(ctx, "**/Start Tournament")) {
                    return false;
                }
                ctx->Yield(10);
                // Wait for tournament to start
                return waitForTournamentRunning(ctx, 10.0f);

            case StartAction::SwitchToView:
                return safeItemClick(ctx, "**/Switch to Tournament View");

            case StartAction::StayInChatbot:
                return safeItemClick(ctx, "**/Stay in Chatbot");

            case StartAction::Cancel:
                return safeItemClick(ctx, "**/Cancel");
        }
        return false;
    }

} // namespace QaplaTest::TournamentChatbot

#endif // IMGUI_ENABLE_TEST_ENGINE
