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

#include <optional>
#include <string>
#include <vector>

namespace QaplaLlm::Actions {

/**
 * @brief One parameter to tune, and the range CLOP may search within.
 *
 * The range is the caller's, not the engine's: an engine that accepts Hash between 1 and 32000
 * says nothing about which slice is worth spending games on, and a CLOP run over the full
 * declared range of a parameter it knows nothing about is games spent to learn what a glance at
 * the engine would have said.
 */
struct ClopParameter {
    std::string name;
    double minimum = 0.0;
    double maximum = 0.0;
};

/**
 * @brief Everything the CLOP run can be told, each field optional and applied on its own.
 *
 * Patch semantics throughout, like SprtSettings: a field left unset keeps whatever is configured,
 * so a caller changing one number does not have to restate the rest.
 */
struct ClopSettings {
    /** @brief Catalog name of the engine whose parameters are tuned. */
    std::optional<std::string> engine;

    /** @brief Catalog names of the engines it plays against, in round-robin order. */
    std::vector<std::string> opponents;

    /**
     * @brief The parameters to tune. Replaces the whole list rather than merging into it.
     *
     * Merging would be the odd one out here: the parameter list is what the run *is*, and a
     * caller sending a shorter list means to tune fewer parameters, not to leave the dropped ones
     * where they were.
     */
    std::vector<ClopParameter> parameters;

    /**
     * @brief The clock both sides play at, e.g. "20+0.1".
     *
     * CLOP itself has none: the games run at whatever each engine configuration carries, so this
     * is written onto all of them at once. Without it the run schedules games and dies on a
     * worker thread -- see ClopData::isReadyToStart().
     */
    std::optional<std::string> timeControl;

    std::optional<std::string> openingsFile;
    std::optional<uint32_t> samples;
    std::optional<uint32_t> gamesPerSample;
    std::optional<uint32_t> warmupSamples;
    std::optional<uint32_t> maxActivePairs;
    std::optional<uint32_t> concurrency;
    std::optional<uint32_t> seed;
    std::optional<double> h;
    std::optional<double> priorVariance;
};

/** @brief Applies the given settings and reports the resulting configuration in full. */
[[nodiscard]] ActionResult configureClop(const ClopSettings& settings);

/** @brief Starts the CLOP run as configured. */
[[nodiscard]] ActionResult startClop();

/** @brief Stops a running CLOP run. Both modes end it; CLOP schedules no new sample either way. */
[[nodiscard]] ActionResult stopClop(StopMode mode);

/** @brief The full configuration, the run state, and the numbers so far. */
[[nodiscard]] ActionResult clopStatus();

/** @brief The status table and the current parameter estimate, as a control in the chat. */
[[nodiscard]] ActionResult showClopResult();

/** @brief Discards the samples collected so far, stopping the run first if one is going. */
[[nodiscard]] ActionResult clearClopResult();

/** @brief One clause for the cross-activity summary, or "" when idle. */
[[nodiscard]] std::string clopActivityText();

/** @brief What the run is doing, for QaplaLlm::ActivityWatch. */
[[nodiscard]] ActivityProgress clopProgress();

/** @brief Whether a start would go through exactly as things stand. */
[[nodiscard]] bool clopIsReadyToStart();

} // namespace QaplaLlm::Actions
