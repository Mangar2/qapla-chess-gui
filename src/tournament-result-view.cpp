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

std::string TournamentResultView::formatHtml(const TournamentResult &result, const std::string &title, bool includePairwise, const TournamentResultView::TournamentMetadata* metadata)
{
    // Work on a non-const copy when we need to call non-const helpers
    TournamentResult r = result;

    std::ostringstream oss;
    
    // HTML header with Arena-style CSS
    oss << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
        << "<html><head>\n"
        << "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n"
        << "<title>" << escapeHtml(title) << "</title>\n"
        << "<style type=\"text/css\">\n"
        << "<!--\n"
        << "body { font-family: Verdana, Arial, Helvetica, sans-serif; font-size: 10pt; background-color: white; }\n"
        << "table.tbstyle { font-size: 10pt; }\n"
        << "td { border-width: 1px; padding: 1px; border-style: solid; border-color: black; }\n"
        << "th { border-width: 1px; padding: 1px; border-style: solid; border-color: black; font-weight: bold; text-align: center; }\n"
        << ".label { font-weight: bold; }\n"
        << "-->\n"
        << "</style>\n"
        << "</head>\n"
        << "<body>\n";

    // Title
    oss << "<h1>" << escapeHtml(title) << "</h1>\n\n";

    // Compute rated list (ranked by score, then S-B)
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

    // Build the main tournament table
    oss << "<table class=\"tbstyle\" border=\"1\" cellspacing=\"0\" cellpadding=\"2\">\n";
    
    // Table header with abbreviated column names for pairwise matches
    oss << "<tr>";
    oss << "<th>Rank</th>";
    oss << "<th>Engine</th>";
    oss << "<th>&nbsp;Score&nbsp;</th>";
    oss << "<th>%</th>";
    
    if (includePairwise) {
        // Add abbreviated engine names as column headers
        for (const auto &n : names) {
            // Create abbreviation from first 2 letters of each word
            std::string abbrev;
            std::istringstream iss(n);
            std::string word;
            while (iss >> word && abbrev.length() < 6) {
                if (word.length() >= 2) {
                    abbrev += word.substr(0, 2);
                } else if (word.length() == 1) {
                    abbrev += word;
                }
            }
            if (abbrev.empty()) {
                abbrev = n.length() >= 2 ? n.substr(0, 2) : n;
            }
            oss << std::format("<th>{}</th>", escapeHtml(abbrev));
        }
    }
    
    oss << "<th>S-B</th>";
    oss << "</tr>\n";

    // Table rows
    int rank = 1;
    const int maxRank = 99;
    for (const auto &s : list) {
        double pct = s.score * 100.0;
        double pts = pointsFromScore(s);
        int totalGames = static_cast<int>(s.total);
        
        oss << "<tr>";
        
        // Rank (formatted with leading zero if < 10)
        if (rank <= maxRank) {
            oss << std::format("<td align=\"right\"><b>{:02d}</b></td>", rank);
        } else {
            oss << std::format("<td align=\"right\"><b>{}</b></td>", rank);
        }
        rank++;
        
        // Engine name (left-aligned)
        oss << std::format("<td>{}</td>", escapeHtml(s.engineName));
        
        // Score (right-aligned)
        oss << std::format("<td align=\"right\">{:.1f}/{}</td>", pts, totalGames);
        
        // Percentage (right-aligned)
        oss << std::format("<td align=\"right\">{:.1f}</td>", pct);
        
        // Pairwise results
        if (includePairwise) {
            for (const auto &other : names) {
                if (other == s.engineName) {
                    // Self-match: use centered dots
                    oss << "<td align=\"center\">&middot; &middot; &middot; &middot; &middot;</td>";
                    continue;
                }
                
                auto itRow = duelsMap.find(s.engineName);
                std::string cell;
                if (itRow != duelsMap.end()) {
                    auto it = itRow->second.find(other);
                    if (it != itRow->second.end()) {
                        const auto &d = it->second;
                        cell = std::format("{}-{}-{}", d.winsEngineA, d.draws, d.winsEngineB);
                    }
                }
                
                if (cell.empty()) {
                    // No games played
                    oss << "<td align=\"center\">&middot; &middot; &middot; &middot; &middot;</td>";
                } else {
                    oss << std::format("<td align=\"center\">{}</td>", cell);
                }
            }
        }
        
        // S-B score (right-aligned with 2 decimals)
        oss << std::format("<td align=\"right\">{:.2f}</td>", sbScores[s.engineName]);
        
        oss << "</tr>\n";
    }
    
    oss << "</table>\n\n";

    // Footer with tournament statistics
    double games = 0.0;
    for (const auto &s : list) { games += s.total; }
    // every game is counted twice (once per engine) so divide by 2 to get actual number
    int totalGames = static_cast<int>(std::round(games / 2.0));
    
    oss << std::format("<p><b>{} games played", totalGames);
    if (metadata != nullptr && metadata->tournamentFinished) {
        oss << " / Tournament finished";
    }
    oss << "</b></p>\n\n";

    // Additional metadata if provided
    if (metadata != nullptr) {
        oss << "<p>\n";
        
        if (!metadata->startTime.empty()) {
            oss << std::format("<b>Tournament start:</b> {}<br>\n", escapeHtml(metadata->startTime));
        }
        
        if (!metadata->latestUpdate.empty()) {
            oss << std::format("<b>Latest update:</b> {}<br>\n", escapeHtml(metadata->latestUpdate));
        }
        
        if (!metadata->site.empty() || !metadata->country.empty()) {
            oss << "<b>Site/ Country:</b> ";
            if (!metadata->site.empty()) {
                oss << escapeHtml(metadata->site);
            }
            if (!metadata->country.empty()) {
                if (!metadata->site.empty()) {
                    oss << ", ";
                }
                oss << escapeHtml(metadata->country);
            }
            oss << "<br>\n";
        }
        
        if (!metadata->level.empty()) {
            oss << std::format("<b>Level:</b> {}<br>\n", escapeHtml(metadata->level));
        }
        
        if (!metadata->hardware.empty()) {
            oss << std::format("<b>Hardware:</b> {}<br>\n", escapeHtml(metadata->hardware));
        }
        
        if (!metadata->operatingSystem.empty()) {
            oss << std::format("<b>Operating system:</b> {}<br>\n", escapeHtml(metadata->operatingSystem));
        }
        
        if (!metadata->pgnFile.empty()) {
            oss << std::format("<b>PGN File:</b> <a href=\"{}\">{}</a><br>\n", 
                              escapeHtml(metadata->pgnFile), escapeHtml(metadata->pgnFile));
        }
        
        if (!metadata->tableCreator.empty()) {
            oss << std::format("<b>Table created with:</b> <a href=\"https://github.com/Mangar2/qapla-chess-gui\">{}</a><br>\n",
                              escapeHtml(metadata->tableCreator));
        }
        
        oss << "</p>\n";
    }

    oss << "</body></html>\n";
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
