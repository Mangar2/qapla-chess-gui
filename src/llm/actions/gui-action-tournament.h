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

#include "gui-action-types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace QaplaLlm::Actions {

/**
 * @brief A patch of the classic (multi-engine round robin) tournament configuration.
 *
 * Every field is optional and independent: an unset field is left exactly as it was, whether it
 * was set earlier this session or restored from a previous one. That is a property of the action,
 * not of any particular wire format -- the model-facing tool happens to expose the same
 * everything-optional shape today, but it could just as well split this across several tools.
 */
struct TournamentSettings {
    std::optional<std::string> timeControl;
    std::optional<uint32_t> gamesPerPairing;
    std::optional<uint32_t> rounds;
    std::optional<std::string> event;

    /** @brief Openings book to use. Rejected (with the current one kept) if the file is missing. */
    std::optional<std::string> openingsFile;

    /**
     * @brief Ask the user for the openings book via a native file picker instead.
     *
     * Takes precedence over openingsFile when both are set, and makes the whole call block on a
     * human -- see ActionResult::endsTurn for what that implies for the caller.
     */
    bool pickOpeningsFile = false;

    std::optional<std::string> pgnFile;

    /** @brief Ask the user for the PGN output file via a native save dialog. See pickOpeningsFile. */
    bool pickPgnFile = false;

    std::optional<uint32_t> concurrency;

    std::optional<AdjudicationMode> drawMode;
    std::optional<uint32_t> drawMinFullMoves;
    std::optional<uint32_t> drawRequiredConsecutiveMoves;
    std::optional<int> drawCentipawnThreshold;

    std::optional<AdjudicationMode> resignMode;
    std::optional<uint32_t> resignRequiredConsecutiveMoves;
    std::optional<int> resignCentipawnThreshold;
    std::optional<bool> resignTwoSided;
};

/**
 * @brief Replaces the tournament's engine selection with the named engines, resolved against the
 * global engine catalog (see resolveEngines() in gui-tool-tournament.h for the matching rules).
 *
 * Fails without changing anything if a name is ambiguous or nothing matched at all; names that
 * simply are not installed are skipped and reported alongside the ones that were selected.
 */
[[nodiscard]] ActionResult selectTournamentEngines(const std::vector<std::string>& engineNames);

/**
 * @brief Applies a TournamentSettings patch, then reports the full resulting configuration.
 *
 * Values the tournament cannot accept (a games count below 1, a missing openings file) are
 * reported as problems while every other field in the same patch is still applied -- one bad
 * field never discards the rest.
 */
[[nodiscard]] ActionResult configureTournament(const TournamentSettings& settings);

/**
 * @brief Starts the tournament, reporting exactly which precondition is missing if it can't.
 *
 * TournamentData reports its own refusals only via the snackbar (see mayStartTournament()), so
 * that is where the reason is recovered from.
 */
[[nodiscard]] ActionResult startTournament();

/** @brief Stops the running tournament; fails if none is running. */
[[nodiscard]] ActionResult stopTournament(StopMode mode);

/** @brief Reports the tournament's full current configuration and run state. */
[[nodiscard]] ActionResult tournamentStatus();

/** @brief Discards the tournament's results, stopping it first if it is still running. */
[[nodiscard]] ActionResult clearTournamentResult();

/** @brief Shows the tournament standings as a live ImGui table in the chat. */
[[nodiscard]] ActionResult showTournamentResult();

/**
 * @brief Names what the tournament is doing right now, or "" when it is idle.
 *
 * Used to describe the tournament as one of several independent activities (see
 * runningActivitiesText() in gui-action-activity.h); callers drop the empty string rather than
 * this special-casing an "idle" wording that only reads well on its own.
 */
[[nodiscard]] std::string tournamentActivityText();

} // namespace QaplaLlm::Actions
