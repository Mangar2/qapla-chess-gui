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
 * @brief A patch of the EPD analysis configuration.
 *
 * EPD analysis tests any number of engines against a fixed set of positions, checking whether
 * each finds the expected best move within a time budget. It shares nothing with the tournament
 * or SPRT configurations -- notably it has no clock string at all (just plain per-position
 * seconds) and no adjudication concept. Unset fields are left untouched.
 */
struct EpdSettings {
    /** @brief EPD (or RAW position) file. Rejected, keeping the current one, if it is missing. */
    std::optional<std::string> epdFile;

    /** @brief Ask the user for the EPD file via a native file picker. See ActionResult::endsTurn. */
    bool pickEpdFile = false;

    /** @brief Upper bound on seconds an engine may search each position. */
    std::optional<uint64_t> maxTimeInSeconds;

    /**
     * @brief Lower bound on seconds an engine must keep searching each position even after it
     * appears to have found the right move -- guards against solving by luck on a shallow search.
     */
    std::optional<uint64_t> minTimeInSeconds;

    /**
     * @brief Consecutive plies the engine's PV must keep showing the correct move before the
     * position counts as solved and the analysis moves on early.
     */
    std::optional<uint32_t> seenPlies;

    std::optional<uint32_t> concurrency;
};

/**
 * @brief Replaces the EPD analysis's engine selection, resolved against the global engine catalog.
 *
 * Unlike SPRT there is no champion/challenger role: every engine is tested against the same
 * position set side by side, and any number of them (including one) is fine.
 */
[[nodiscard]] ActionResult selectEpdEngines(const std::vector<std::string>& engineNames);

/** @brief Applies an EpdSettings patch and reports what it changed. */
[[nodiscard]] ActionResult configureEpd(const EpdSettings& settings);

/**
 * @brief Starts -- or resumes -- the EPD analysis.
 *
 * A single call covers both: EpdData::analyse() decides internally which one this is. If the
 * configured engines, EPD file and timing haven't changed since the last run, it picks up where
 * it left off; if that run already completed, or its settings changed after it stopped, starting
 * fails until the results are cleared.
 */
[[nodiscard]] ActionResult startEpd();

/**
 * @brief Stops the running EPD analysis; fails if none is running.
 *
 * Progress is kept, not cleared -- starting again resumes from here.
 */
[[nodiscard]] ActionResult stopEpd(StopMode mode);

/** @brief Reports the EPD analysis's full current configuration, run state and progress. */
[[nodiscard]] ActionResult epdStatus();

/** @brief Discards the EPD analysis results, stopping it first if it is still running. */
[[nodiscard]] ActionResult clearEpdResult();

/** @brief Shows the per-position solved/not-solved table as a live ImGui table in the chat. */
[[nodiscard]] ActionResult showEpdResult();

/** @brief Names what the EPD analysis is doing right now, or "" when it is idle. */
[[nodiscard]] std::string epdActivityText();

} // namespace QaplaLlm::Actions
