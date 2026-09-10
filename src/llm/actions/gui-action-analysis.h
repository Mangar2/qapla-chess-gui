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
 * @brief A patch of the backward analysis configuration.
 *
 * The backward analysis recomputes the games of a PGN file, each from its last move back to its
 * first, and writes them out again with the evaluations the engine found. It shares nothing with
 * the tournament, SPRT or EPD configurations: it plays no games, it has no clock, and its only
 * search limit is one fixed time every position gets. Unset fields are left untouched.
 */
struct AnalysisSettings {
    /** @brief The PGN file holding the games to analyse. Rejected if it is not there. */
    std::optional<std::string> pgnFile;

    /** @brief Milliseconds the engine may think about each position. */
    std::optional<uint64_t> moveTimeMs;

    /** @brief The PGN file the analysed games are written to. */
    std::optional<std::string> outputFile;

    /** @brief True to append to the output file, false to overwrite it when the run starts. */
    std::optional<bool> appendOutput;

    /** @brief How many games are recomputed at the same time. */
    std::optional<uint32_t> concurrency;
};

/**
 * @brief Replaces the engines of the backward analysis, resolved against the engine catalog.
 *
 * Every engine analyses every game, and each writes its own copy of it -- told apart by the
 * Annotator tag. Any number is fine, including one.
 */
[[nodiscard]] ActionResult selectAnalysisEngines(const std::vector<std::string>& engineNames);

/** @brief Applies an AnalysisSettings patch, then reports the full resulting configuration. */
[[nodiscard]] ActionResult configureAnalysis(const AnalysisSettings& settings);

/**
 * @brief Starts the backward analysis, reading the games from the configured PGN file.
 *
 * Reading them here is what makes the run possible without a screen. The chatbot hands over what
 * the Pgn view holds instead, so that what is analysed is what the user has just filtered; a
 * caller over HTTP has no such view and gets the whole file.
 */
[[nodiscard]] ActionResult startAnalysis();

/** @brief Stops the running backward analysis; fails if none is running. */
[[nodiscard]] ActionResult stopAnalysis(StopMode mode);

/** @brief Reports the backward analysis's configuration, run state and progress. */
[[nodiscard]] ActionResult analysisStatus();

/** @brief What the backward analysis is doing, for an onlooker. */
[[nodiscard]] ActivityProgress analysisProgress();

/** @brief Whether a start would succeed as things stand. */
[[nodiscard]] bool analysisIsReadyToStart();

/**
 * @brief Forgets the games of the last run, stopping it first if it is still going.
 *
 * The analysed games themselves are not touched: they are in the output file, which is the
 * point of the run and not this application's to delete.
 */
[[nodiscard]] ActionResult clearAnalysisResult();

/** @brief The names the shared status sentences use for this activity. */
inline constexpr ActivityNames ANALYSIS_NAMES{
    .withArticle = "a backward analysis", .bare = "backward analysis", .workItems = "games"};

} // namespace QaplaLlm::Actions
