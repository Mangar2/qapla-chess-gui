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
#include <optional>

namespace QaplaTest::TournamentChatbot {

    /**
     * @brief Actions for StopRunning step
     */
    enum class StopRunningAction {
        EndTournament,  // Click "Yes, end tournament"
        Cancel          // Click "Cancel" - keeps tournament running
    };

    /**
     * @brief Executes the StopRunning step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @return true if step executed successfully
     * 
     * Precondition: Tournament is running
     * Postcondition: 
     *   - EndTournament: Tournament stopped, proceeds to Menu step
     *   - Cancel: Tournament continues, chatbot closes
     */
    bool executeStopRunningStep(ImGuiTestContext* ctx, StopRunningAction action);

    /**
     * @brief Actions for ContinueExisting step
     */
    enum class ContinueExistingAction {
        YesContinue,  // Click "Yes, continue tournament"
        No,           // Click "No" - go to Menu
        Cancel        // Click "Cancel" - exit chatbot
    };

    /**
     * @brief Executes the ContinueExisting step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @return true if step executed successfully
     * 
     * Precondition: Tournament has scheduled tasks but is not running
     * Postcondition:
     *   - YesContinue: Goes to Start step
     *   - No: Goes to Menu step
     *   - Cancel: Chatbot closes
     */
    bool executeContinueExistingStep(ImGuiTestContext* ctx, ContinueExistingAction action);

    /**
     * @brief Actions for Menu step
     */
    enum class MenuAction {
        NewTournament,   // Click "New tournament"
        SaveTournament,  // Click "Save tournament" (opens file dialog)
        LoadTournament,  // Click "Load tournament" (opens file dialog)
        Cancel           // Click "Cancel"
    };

    /**
     * @brief Executes the Menu step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @return true if step executed successfully
     * 
     * Postcondition:
     *   - NewTournament: Tournament cleared, goes to GlobalSettings
     *   - SaveTournament: Opens save dialog, stays in Menu (unless file selected)
     *   - LoadTournament: Opens load dialog, goes to Start if file selected
     *   - Cancel: Chatbot closes
     */
    bool executeMenuStep(ImGuiTestContext* ctx, MenuAction action);

    /**
     * @brief Actions for GlobalSettings step
     */
    enum class GlobalSettingsAction {
        Continue,     // Click "Continue"
        MoreOptions,  // Click "More Options"
        LessOptions,  // Click "Less Options"
        Cancel        // Click "Cancel"
    };

    /**
     * @brief Executes the GlobalSettings step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @return true if step executed successfully
     */
    bool executeGlobalSettingsStep(ImGuiTestContext* ctx, GlobalSettingsAction action);

    /**
     * @brief Actions for SelectEngines step
     */
    enum class SelectEnginesAction {
        Continue,  // Click "Continue"
        Cancel     // Click "Cancel"
    };

    /**
     * @brief Executes the SelectEngines step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @param selectEngines If true, select engines before action
     * @return true if step executed successfully
     */
    bool executeSelectEnginesStep(ImGuiTestContext* ctx, SelectEnginesAction action, bool selectEngines = true);

    /**
     * @brief Actions for LoadEngine step
     */
    enum class LoadEngineAction {
        AddEngines,       // Click "Add Engines" (opens file dialog)
        DetectContinue,   // Click "Detect & Continue"
        SkipDetection,    // Click "Skip Detection"
        Continue,         // Click "Continue" (when detection not needed)
        Cancel            // Click "Cancel"
    };

    /**
     * @brief Executes the LoadEngine step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @return true if step executed successfully
     */
    bool executeLoadEngineStep(ImGuiTestContext* ctx, LoadEngineAction action);

    /**
     * @brief Actions for Configuration step
     */
    enum class ConfigurationAction {
        Continue,     // Click "Continue"
        MoreOptions,  // Click "More Options"
        LessOptions,  // Click "Less Options"
        Cancel        // Click "Cancel"
    };

    /**
     * @brief Executes the Configuration step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @return true if step executed successfully
     */
    bool executeConfigurationStep(ImGuiTestContext* ctx, ConfigurationAction action);

    /**
     * @brief Actions for Opening step
     */
    enum class OpeningAction {
        Continue,     // Click "Continue" (requires valid opening file)
        MoreOptions,  // Click "More Options"
        LessOptions,  // Click "Less Options"
        ShowTrace,    // Click "Show Trace"
        HideTrace,    // Click "Hide Trace"
        Cancel        // Click "Cancel"
    };

    /**
     * @brief Executes the Opening step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @param setupOpeningFile If true, sets a valid opening file before action
     * @return true if step executed successfully
     */
    bool executeOpeningStep(ImGuiTestContext* ctx, OpeningAction action, bool setupOpeningFile = true);

    /**
     * @brief Actions for PGN step
     */
    enum class PgnAction {
        Continue,           // Click "Continue"
        OverwriteContinue,  // Click "Overwrite & Continue"
        MoreOptions,        // Click "More Options"
        LessOptions,        // Click "Less Options"
        Cancel              // Click "Cancel"
    };

    /**
     * @brief Executes the PGN step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @param setupPgnFile If true, sets a valid PGN file path before action
     * @param appendMode If specified, sets append mode (true=Append, false=Overwrite)
     * @return true if step executed successfully
     */
    bool executePgnStep(ImGuiTestContext* ctx, PgnAction action, bool setupPgnFile = true, 
                        std::optional<bool> appendMode = std::nullopt);

    /**
     * @brief Actions for Start step
     */
    enum class StartAction {
        StartTournament,     // Click "Start Tournament"
        SwitchToView,        // Click "Switch to Tournament View" (after start)
        StayInChatbot,       // Click "Stay in Chatbot" (after start)
        Cancel               // Click "Cancel"
    };

    /**
     * @brief Executes the Start step with specified action
     * @param ctx Test context
     * @param action Which button to click
     * @return true if step executed successfully
     */
    bool executeStartStep(ImGuiTestContext* ctx, StartAction action);

} // namespace QaplaTest::TournamentChatbot

#endif // IMGUI_ENABLE_TEST_ENGINE
