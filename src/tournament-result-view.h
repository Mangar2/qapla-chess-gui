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

#include <string>
#include <vector>
#include <unordered_map>

namespace QaplaTester { class TournamentResult; }

namespace QaplaWindows {

/**
 * @brief Utility class to render tournament results in different formats.
 *
 * Provides export helpers for HTML, CSV and plain text. The HTML exporter
 * attempts to mimic the Arena-style table layout used in the UI screenshots.
 */
class TournamentResultView {
public:
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
     * @return A string containing the complete HTML output.
     */
    static std::string formatHtml(const QaplaTester::TournamentResult &result,
                                  const std::string &title = "Tournament",
                                  bool includePairwise = true);

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
};

} // namespace QaplaWindows
