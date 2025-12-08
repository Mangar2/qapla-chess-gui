/**
 * @license
 * See header.
 */

#include "tournament-result-view.h"
#include "tournament-result.h"

#include <sstream>
#include <format>
#include <cmath>

namespace QaplaWindows {

using namespace QaplaTester;

static double pointsFromScore(const TournamentResult::Scored &s) {
    return s.score * s.total;
}

static std::string escapeHtml(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += c; break;
        }
    }
    return out;
}

std::string TournamentResultView::formatHtml(const TournamentResult &result, const std::string &title, bool includePairwise)
{
    // Work on a non-const copy when we need to call non-const helpers
    TournamentResult r = result;

    std::ostringstream oss;
    oss << "<!doctype html><html><head><meta charset=\"utf-8\"><style>"
        << "table{border-collapse:collapse;font-family:sans-serif;} th,td{border:1px solid #000;padding:4px;text-align:center;}"
        << "th{background:#eee;font-weight:bold;}"
        << "</style></head><body>";

    oss << std::format("<h1>{}</h1>", escapeHtml(title));

    // Compute rated list (ranked by ELO as computed by the helper)
    std::vector<TournamentResult::Scored> list = r.computeAllElos(2600, 50, false);

    // Map for quick lookups
    std::unordered_map<std::string, TournamentResult::Scored> scoredMap;
    std::vector<std::string> names;
    for (const auto &s : list) {
        scoredMap[s.engineName] = s;
        names.push_back(s.engineName);
    }

    // Build a map for pair-wise duels: map[name][opponent] -> EngineDuelResult
    std::unordered_map<std::string, std::unordered_map<std::string, EngineDuelResult>> duelsMap;
    for (const auto &name : names) {
        if (auto opt = r.forEngine(name)) {
            for (const auto &d : opt->duels) {
                duelsMap[name][d.getEngineB()] = d;
            }
        }
    }

    // Compute S-B (Sonneborn–Berger) score
    std::unordered_map<std::string, double> totalPoints;
    for (const auto &s : list) {
        totalPoints[s.engineName] = pointsFromScore(s);
    }

    std::unordered_map<std::string, double> sbScores;
    for (const auto &s : list) {
        double sb = 0.0;
        if (auto opt = r.forEngine(s.engineName)) {
            for (const auto &d : opt->duels) {
                // points earned by s against this opponent
                double pts = d.winsEngineA + 0.5 * d.draws;
                double oppPoints = totalPoints[d.getEngineB()];
                sb += pts * oppPoints;
            }
        }
        sbScores[s.engineName] = sb;
    }

    // Outer table header (rank + engine + score)
    oss << "<table><tr><th>Rank</th><th>Engine</th><th>Score</th><th>%</th><th>S-B</th>";
    if (includePairwise) {
        for (const auto &n : names) {
            oss << std::format("<th>{}</th>", escapeHtml(n));
        }
    }
    oss << "</tr>";

    int rank = 1;
    for (const auto &s : list) {
        // Inline computation to avoid unused-variable warnings
        double pct = s.score * 100.0;
        oss << "<tr>";
        oss << std::format("<td>{}</td>", rank++);
        oss << std::format("<td style='text-align:left'>{}</td>", escapeHtml(s.engineName));
        oss << std::format("<td>{:.1f}/{}</td>", pointsFromScore(s), static_cast<int>(s.total));
        oss << std::format("<td>{:.1f}</td>", pct);
        oss << std::format("<td>{:.2f}</td>", sbScores[s.engineName]);

        if (includePairwise) {
            for (const auto &other : names) {
                if (other == s.engineName) {
                    oss << "<td>...</td>";
                    continue;
                }
                auto itRow = duelsMap.find(s.engineName);
                std::string cell = "-";
                if (itRow != duelsMap.end()) {
                    auto it = itRow->second.find(other);
                    if (it != itRow->second.end()) {
                        const auto &d = it->second;
                        cell = std::format("{}-{}-{}", d.winsEngineA, d.draws, d.winsEngineB);
                    }
                }
                oss << std::format("<td>{}</td>", cell);
            }
        }
        oss << "</tr>";
    }
    oss << "</table>";

    // Footer with basic stats
    double games = 0.0;
    for (const auto &s : list) { games += s.total; }
    // every game is counted twice (once per engine) so divide by 2 to get actual number
    int totalGames = static_cast<int>(std::round(games / 2.0));
    oss << std::format("<p>{} games played</p>", totalGames);

    oss << "</body></html>";
    return oss.str();
}

std::string TournamentResultView::formatPlainText(const TournamentResult &result, int averageElo)
{
    std::ostringstream oss;
    oss << "Tournament result:\n";
    TournamentResult r = result;
    r.printRatingTableUciStyle(oss, averageElo);
    return oss.str();
}

std::string TournamentResultView::formatCsv(const TournamentResult &result, int averageElo)
{
    std::ostringstream oss;
    oss << "Rank,Engine,Elo,Error,Games,Score%,Points\n";
    TournamentResult r = result;
    auto list = r.computeAllElos(averageElo, 50, false);
    int rank = 1;
    for (const auto &s : list) {
        oss << std::format("{},{},{:.0f},{},{},{:.2f}\n",
                   rank++, s.engineName, s.elo, s.error, static_cast<int>(s.total), s.score * 100.0);
    }
    return oss.str();
}

} // namespace QaplaWindows
