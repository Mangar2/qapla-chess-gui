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

namespace QaplaLlm::Actions {

/**
 * @brief A patch of the SPRT (Sequential Probability Ratio Test) configuration.
 *
 * SPRT compares exactly two engines -- a champion (comparison baseline) and a challenger (engine
 * under test). It keeps its own copy of every setting that merely looks like a tournament setting
 * (time control, concurrency, openings, PGN output, adjudication -- see
 * QaplaWindows::SprtTournamentData): configuring one never affects the other. Unset fields are
 * left untouched, exactly as in TournamentSettings.
 */
struct SprtSettings {
    std::optional<std::string> timeControl;

    /** @brief H0 Elo bound: the challenger is NOT stronger at or below this Elo difference. */
    std::optional<double> eloH0;

    /** @brief H1 Elo bound: the challenger IS stronger at or above this Elo difference. */
    std::optional<double> eloH1;

    /** @brief Type I error rate; rejected outside 0..1 exclusive. */
    std::optional<double> alpha;

    /** @brief Type II error rate; rejected outside 0..1 exclusive. */
    std::optional<double> beta;

    std::optional<uint32_t> maxGames;

    /** @brief Statistical model: "normalized", "logistic" or "bayesian"; anything else is rejected. */
    std::optional<std::string> model;

    /** @brief Pentanomial (paired-game) statistics instead of trinomial. */
    std::optional<bool> pentanomial;

    std::optional<std::string> openingsFile;

    /** @brief Ask the user for the openings book via a native file picker. See ActionResult::endsTurn. */
    bool pickOpeningsFile = false;

    std::optional<std::string> pgnFile;

    /** @brief Ask the user for the PGN output file via a native save dialog. */
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
 * @brief Sets the SPRT duel's engines, resolved against the global engine catalog.
 *
 * Each role is a patch, like every other setting: a role given by name is replaced, a role left
 * unset keeps whatever engine is configured for it. Naming only one is the common case -- a new
 * build against the same baseline -- so requiring both would reject a perfectly complete request.
 *
 * Fails without changing anything if a given name is ambiguous or not installed, if the role that
 * was not given has no engine configured yet, or if the resulting pair would be one engine twice.
 *
 * Reports nothing on success: callers are expected to follow it with configureSprt(), whose status
 * already names champion and challenger (see selectTournamentEngines() for the same reasoning).
 */
[[nodiscard]] ActionResult selectSprtEngines(const std::optional<std::string>& championName,
    const std::optional<std::string>& challengerName);

/** @brief Applies an SprtSettings patch, then reports the full resulting configuration. */
[[nodiscard]] ActionResult configureSprt(const SprtSettings& settings);

/** @brief Starts the SPRT test, reporting exactly which precondition is missing if it can't. */
[[nodiscard]] ActionResult startSprt();

/** @brief Stops the running SPRT test; fails if none is running. */
[[nodiscard]] ActionResult stopSprt(StopMode mode);

/** @brief Reports the SPRT test's full current configuration and run state. */
[[nodiscard]] ActionResult sprtStatus();

/** @brief Discards the SPRT results, stopping the test first if it is still running. */
[[nodiscard]] ActionResult clearSprtResult();

/** @brief Shows the SPRT decision table and the raw duel score as live ImGui tables in the chat. */
[[nodiscard]] ActionResult showSprtResult();

/** @brief Names what the SPRT test is doing right now, or "" when it is idle. */
[[nodiscard]] std::string sprtActivityText();

/** @brief Whether the SPRT test could be started exactly as configured. */
[[nodiscard]] bool sprtIsReadyToStart();

} // namespace QaplaLlm::Actions
