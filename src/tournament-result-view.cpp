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

namespace {

double pointsFromScore(const TournamentResult::Scored &s) {
    return s.score * s.total;
}

std::string escapeHtml(const std::string &s) {
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

} // unnamed namespace

std::string TournamentResultView::abbreviateEngineName(const std::string &name) {
    // Use only first 2 letters
    if (name.length() >= 2) {
        return name.substr(0, 2);
    }
    return name;
}

std::unordered_map<std::string, std::unordered_map<std::string, EngineDuelResult>> TournamentResultView::buildDuelsMap(
    const std::vector<std::string> &names,
    const TournamentResult &result) {
    
    decltype(buildDuelsMap(names, result)) duelsMap;
    
    for (const auto &name : names) {
        if (auto opt = result.forEngine(name)) {
            for (const auto &duel : opt->duels) {
                duelsMap[name][duel.getEngineB()] = duel;
            }
        }
    }
    return duelsMap;
}

void TournamentResultView::computeSonnebornBerger(
    const std::vector<TournamentResult::Scored> &list,
    const TournamentResult &result,
    std::unordered_map<std::string, double> &sbScores) {
    
    std::unordered_map<std::string, double> totalPoints;
    for (const auto &scored : list) {
        totalPoints[scored.engineName] = pointsFromScore(scored);
    }
    
    for (const auto &scored : list) {
        double snb = 0.0;
        if (auto opt = result.forEngine(scored.engineName)) {
            for (const auto &duel : opt->duels) {
                double pts = duel.winsEngineA + 0.5 * duel.draws;
                double oppPoints = totalPoints[duel.getEngineB()];
                snb += pts * oppPoints;
            }
        }
        sbScores[scored.engineName] = snb;
    }
}

std::string TournamentResultView::formatPairwiseResult(
    const std::string &engineName,
    const std::string &opponent,
    const std::unordered_map<std::string, std::unordered_map<std::string, EngineDuelResult>> &duelsMap) {
    
    if (engineName == opponent) {
        return "· · · · ·";
    }
    
    auto itRow = duelsMap.find(engineName);
    if (itRow != duelsMap.end()) {
        auto itCol = itRow->second.find(opponent);
        if (itCol != itRow->second.end()) {
            const auto &duel = itCol->second;
            return std::format("{}-{}-{}", duel.winsEngineA, duel.draws, duel.winsEngineB);
        }
    }
    
    return "· · · · ·";
}

namespace {

void writeHtmlHeader(std::ostringstream &oss, const std::string &title) {
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
    oss << "<h1>" << escapeHtml(title) << "</h1>\n\n";
}

void writeTableHeader(std::ostringstream &oss, bool includePairwise, const std::vector<std::string> &names) {
    oss << "<table class=\"tbstyle\" border=\"1\" cellspacing=\"0\" cellpadding=\"2\">\n";
    oss << "<tr>";
    oss << "<th>Rank</th>";
    oss << "<th>Engine</th>";
    oss << "<th>&nbsp;Score&nbsp;</th>";
    oss << "<th>%</th>";
    
    if (includePairwise) {
        for (const auto &name : names) {
            std::string abbrev = TournamentResultView::abbreviateEngineName(name);
            oss << std::format("<th>{}</th>", escapeHtml(abbrev));
        }
    }
    
    oss << "<th>S-B</th>";
    oss << "</tr>\n";
}

void writePairwiseCell(
    std::ostringstream &oss,
    const std::string &engineName,
    const std::string &opponent,
    const std::unordered_map<std::string, std::unordered_map<std::string, EngineDuelResult>> &duelsMap) {
    
    std::string result = TournamentResultView::formatPairwiseResult(engineName, opponent, duelsMap);
    
    // Convert · to HTML entity for display
    if (result == "· · · · ·") {
        oss << "<td align=\"center\">&middot; &middot; &middot; &middot; &middot;</td>";
    } else {
        oss << std::format("<td align=\"center\">{}</td>", result);
    }
}

void writeTableRow(
    std::ostringstream &oss,
    int rank,
    const TournamentResult::Scored &scored,
    bool includePairwise,
    const std::vector<std::string> &names,
    const std::unordered_map<std::string, std::unordered_map<std::string, EngineDuelResult>> &duelsMap,
    const std::unordered_map<std::string, double> &sbScores) {
    
    const int maxRank = 99;
    double pct = scored.score * 100.0;
    double pts = pointsFromScore(scored);
    int totalGames = static_cast<int>(scored.total);
    
    oss << "<tr>";
    
    if (rank <= maxRank) {
        oss << std::format("<td align=\"right\"><b>{:02d}</b></td>", rank);
    } else {
        oss << std::format("<td align=\"right\"><b>{}</b></td>", rank);
    }
    
    oss << std::format("<td>{}</td>", escapeHtml(scored.engineName));
    oss << std::format("<td align=\"right\">{:.1f}/{}</td>", pts, totalGames);
    oss << std::format("<td align=\"right\">{:.1f}</td>", pct);
    
    if (includePairwise) {
        for (const auto &opponent : names) {
            writePairwiseCell(oss, scored.engineName, opponent, duelsMap);
        }
    }
    
    oss << std::format("<td align=\"right\">{:.2f}</td>", sbScores.at(scored.engineName));
    oss << "</tr>\n";
}

void writeTableBody(
    std::ostringstream &oss,
    const std::vector<TournamentResult::Scored> &list,
    bool includePairwise,
    const std::vector<std::string> &names,
    const std::unordered_map<std::string, std::unordered_map<std::string, EngineDuelResult>> &duelsMap,
    const std::unordered_map<std::string, double> &sbScores) {
    
    int rank = 1;
    for (const auto &scored : list) {
        writeTableRow(oss, rank, scored, includePairwise, names, duelsMap, sbScores);
        rank++;
    }
    
    oss << "</table>\n\n";
}

void writeFooterStatistics(
    std::ostringstream &oss,
    const std::vector<TournamentResult::Scored> &list,
    const TournamentResultView::TournamentMetadata* metadata) {
    
    double gms = 0.0;
    for (const auto &scored : list) {
        gms += scored.total;
    }
    int totalGames = static_cast<int>(std::round(gms / 2.0));
    
    oss << std::format("<p><b>{} games played", totalGames);
    if (metadata != nullptr && metadata->tournamentFinished) {
        oss << " / Tournament finished";
    }
    oss << "</b></p>\n\n";
}

void writeMetadataSection(std::ostringstream &oss, const TournamentResultView::TournamentMetadata* metadata) {
    if (metadata == nullptr) {
        return;
    }
    
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

} // unnamed namespace

std::string TournamentResultView::formatHtml(const TournamentResult &result, const std::string &title, bool includePairwise, const TournamentResultView::TournamentMetadata* metadata)
{
    // Make a mutable copy for computeAllElos
    TournamentResult res = result;
    std::ostringstream oss;
    
    writeHtmlHeader(oss, title);
    
    std::vector<TournamentResult::Scored> lst = res.computeAllElos(2600, 50, false);
    
    std::vector<std::string> nms;
    nms.reserve(lst.size());
    for (const auto &scored : lst) {
        nms.push_back(scored.engineName);
    }
    
    auto dulsMap = TournamentResultView::buildDuelsMap(nms, res);
    
    std::unordered_map<std::string, double> sbScrs;
    TournamentResultView::computeSonnebornBerger(lst, res, sbScrs);
    
    writeTableHeader(oss, includePairwise, nms);
    writeTableBody(oss, lst, includePairwise, nms, dulsMap, sbScrs);
    writeFooterStatistics(oss, lst, metadata);
    writeMetadataSection(oss, metadata);
    
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
