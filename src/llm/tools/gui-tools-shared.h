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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#pragma once

#include "../api/llm-tool-api.h"

#include <string>
#include <vector>

/**
 * @file
 * @brief Parameters shared by more than one tool, declared once.
 *
 * The tournament and SPRT configuration tools expose the same time control and the same
 * adjudication block. The wording of those parameters is the part that gets tuned most often, so
 * it exists in exactly one place -- and since the two settings structs name these fields
 * identically, one template covers both without either side having to hear about the other.
 */

namespace QaplaLlm::Tools {

/** @brief Chess clock time control, bound to `Settings::timeControl`. */
template <class Settings>
[[nodiscard]] Api::Param<Settings> timeControlParam() {
    return Api::stringParam<Settings>("time_control", &Settings::timeControl,
        "Chess clock time control. Format: \"<base>+<increment>\", both in "
        "seconds; increment is optional and defaults to 0. <base> may also "
        "be \"M:S\" or \"H:M:S\". Prefix with \"<moves>/\" for a moves-per-session "
        "phase (e.g. \"40/300+2\" = 40 moves in 300s, then +2s per move "
        "thereafter). Use \"inf\" for no time limit. "
        "Always convert the user's own words into this format yourself -- "
        "never ask the user to supply an already-formatted time control "
        "string. Examples: \"1 minute per game\" -> \"60+0\" (or \"1:00\"); "
        "\"5 minutes plus 3 second increment\" / \"5+3\" -> \"300+3\" (or "
        "\"5:00+3\"); \"40 moves in 5 minutes, then 2 seconds per move\" -> "
        "\"40/300+2\"; \"blitz\" (no further detail given) -> \"300+0\"; "
        "\"unlimited\"/\"no clock\" -> \"inf\".");
}

/** @brief The off/test/active vocabulary both adjudication modes use. */
[[nodiscard]] inline std::vector<std::pair<std::string, Actions::AdjudicationMode>>
adjudicationModeValues() {
    return {{"off", Actions::AdjudicationMode::Off}, {"test", Actions::AdjudicationMode::Test},
        {"active", Actions::AdjudicationMode::Active}};
}

/**
 * @brief Appends the full draw/resign adjudication block, bound to the matching `Settings` fields.
 *
 * Works for any settings struct naming these eight fields the way TournamentSettings and
 * SprtSettings do; that is what lets the two tools share one wording.
 */
template <class Settings>
void appendAdjudicationParams(std::vector<Api::Param<Settings>>& params) {
    params.push_back(Api::enumParam<Settings>("draw_mode", &Settings::drawMode,
        "\"off\": disables draw adjudication. \"test\": evaluates/logs decision, doesn't end "
        "games. \"active\": ends games early as draw once conditions below met.",
        adjudicationModeValues()));
    params.push_back(Api::integerParam<Settings>("draw_min_full_moves", &Settings::drawMinFullMoves,
        "Min full moves before draw adjudication can trigger. Default 80."));
    params.push_back(Api::integerParam<Settings>("draw_required_consecutive_moves",
        &Settings::drawRequiredConsecutiveMoves,
        "Consecutive moves (engines' own eval) that must stay within draw_centipawn_threshold "
        "of equal before draw adjudication. Default 20."));
    params.push_back(Api::integerParam<Settings>("draw_centipawn_threshold",
        &Settings::drawCentipawnThreshold,
        "Max abs eval in centipawns still counting as drawn (e.g. 20 = within +/-20cp of "
        "equal). Positive number. Default 20."));

    params.push_back(Api::enumParam<Settings>("resign_mode", &Settings::resignMode,
        "\"off\": disables resign adjudication. \"test\": evaluates/logs decision, doesn't "
        "end games. \"active\": ends games early as loss for losing side once conditions "
        "below met.",
        adjudicationModeValues()));
    params.push_back(Api::integerParam<Settings>("resign_required_consecutive_moves",
        &Settings::resignRequiredConsecutiveMoves,
        "Consecutive moves whose own eval must stay at/below -resign_centipawn_threshold "
        "(that bad or worse) before resign adjudication. Default 5."));
    params.push_back(Api::integerParam<Settings>("resign_centipawn_threshold",
        &Settings::resignCentipawnThreshold,
        "How bad, in centipawns from losing side's own view, before resignation-worthy -- "
        "e.g. 500 = down ~queen's worth. Positive magnitude, not negative. Default 500."));
    params.push_back(Api::boolParam<Settings>("resign_two_sided", &Settings::resignTwoSided,
        "If true, both engines must independently agree position lost before adjudicating "
        "-- more conservative, avoids resignation from one engine's eval blunder alone. "
        "Default false."));
}

/** @brief Openings book path, bound to `Settings::openingsFile`. */
template <class Settings>
[[nodiscard]] Api::Param<Settings> openingsFileParam(std::string description) {
    return Api::stringParam<Settings>("openings_file", &Settings::openingsFile,
        std::move(description));
}

/** @brief Openings book file-picker opt-in, bound to `Settings::pickOpeningsFile`. */
template <class Settings>
[[nodiscard]] Api::Param<Settings> openingsFileDialogParam() {
    auto param = Api::flagParam<Settings>("openings_file_dialog", &Settings::pickOpeningsFile,
        "Set true to open a native file picker instead of passing openings_file.");
    // Waits on the person at the window to pick a file, so it is not offered to a caller that
    // has no window -- see GuiToolDefinition::localOnlyParameters.
    param.localOnly = true;
    return param;
}

/** @brief PGN output path, bound to `Settings::pgnFile`. */
template <class Settings>
[[nodiscard]] Api::Param<Settings> pgnFileParam() {
    return Api::stringParam<Settings>("pgn_file", &Settings::pgnFile,
        "Path to save played games as PGN.");
}

/** @brief PGN output file-picker opt-in, bound to `Settings::pickPgnFile`. */
template <class Settings>
[[nodiscard]] Api::Param<Settings> pgnFileDialogParam() {
    auto param = Api::flagParam<Settings>("pgn_file_dialog", &Settings::pickPgnFile,
        "Set true to open a native save-file picker instead of passing pgn_file.");
    param.localOnly = true; // see openingsFileDialogParam()
    return param;
}

/**
 * @brief How long a tool that waits on a native file dialog is given.
 *
 * There is no way to know in advance how long a person takes to pick a file, and a short timeout
 * would abandon the call while the handler is still legitimately running, orphaning its
 * (eventually correct) result.
 */
inline constexpr auto FILE_DIALOG_TIMEOUT = std::chrono::minutes(10);

} // namespace QaplaLlm::Tools
