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
 * @author Automatic contribution
 */

#pragma once

#include "tournament-result.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace QaplaWindows {

/**
 * @brief Utility class to render tournament results in different formats.
 *
 * Provides export helpers for HTML, CSV and plain text. The HTML exporter
 * attempts to mimic the Arena-style table layout used in the UI screenshots.
 */
class TournamentResultView {
public:
    /**
     * @brief Metadata for tournament HTML export.
     */
    struct TournamentMetadata {
        std::string startTime;       ///< Tournament start date/time
        std::string latestUpdate;    ///< Last update date/time
        std::string site;            ///< Site/hostname
        std::string country;         ///< Country
        std::string level;           ///< Time control level (e.g., "Blitz 1/1")
        std::string hardware;        ///< Hardware description
        std::string operatingSystem; ///< OS description
        std::string pgnFile;         ///< PGN filename
        std::string tableCreator;    ///< Tool used to create table
        bool tournamentFinished = false; ///< Tournament completion status
    };

    TournamentResultView() = delete;
    ~TournamentResultView() = delete;

    /**
     * @brief Produces an HTML representation of the tournament results.
     *
     * The HTML contains a ranking table ordered by Elo/score and an optional
     * matrix with pair-wise W-D-L counts for each engine pair.
     *
     * @param result Tournament result structure to render
     * @param title Title for the HTML document
     * @param includePairwise If true, a pair-wise duel matrix is produced.
     * @param metadata Optional tournament metadata for footer information.
     * @return A string containing the complete HTML output.
     */
    static std::string formatHtml(const QaplaTester::TournamentResult &result,
                                  const std::string &title = "Tournament",
                                  bool includePairwise = true,
                                  const TournamentMetadata* metadata = nullptr);

    /**
     * @brief Produces a compact plain-text summary similar to `printSummary()`.
     */
    static std::string formatPlainText(const QaplaTester::TournamentResult &result,
                                       int averageElo = 2600);

    /**
     * @brief Produces a CSV representation of the rating table.
     */
    static std::string formatCsv(const QaplaTester::TournamentResult &result,
                                 int averageElo = 2600);

    /**
     * @brief Builds a map of all duel results between engines.
     * @param names List of engine names to process
     * @param result Tournament result to extract duels from
     * @return Map: engineA -> engineB -> duel result
     */
    static std::unordered_map<std::string, std::unordered_map<std::string, QaplaTester::EngineDuelResult>> buildDuelsMap(
        const std::vector<std::string> &names,
        const QaplaTester::TournamentResult &result);

    /**
     * @brief Computes Sonneborn-Berger tiebreak scores for all engines.
     * @param list List of scored engines
     * @param result Tournament result to extract duels from
     * @param sbScores Output map: engineName -> Sonneborn-Berger score
     */
    static void computeSonnebornBerger(
        const std::vector<QaplaTester::TournamentResult::Scored> &list,
        const QaplaTester::TournamentResult &result,
        std::unordered_map<std::string, double> &sbScores);

    /**
     * @brief Abbreviates an engine name to first 2 characters.
     * @param name Full engine name
     * @return Abbreviated name (2 characters or less)
     */
    static std::string abbreviateEngineName(const std::string &name);

    /**
     * @brief Formats the pairwise result between two engines.
     * @param engineName Name of the first engine
     * @param opponent Name of the opponent engine
     * @param duelsMap Map of all duel results
     * @return Formatted string like "2-1-1" or "· · · · ·" for diagonal/missing
     */
    static std::string formatPairwiseResult(
        const std::string &engineName,
        const std::string &opponent,
        const std::unordered_map<std::string, std::unordered_map<std::string, QaplaTester::EngineDuelResult>> &duelsMap);
};

} // namespace QaplaWindows
