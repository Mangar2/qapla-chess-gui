/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "tournament-config-sections.h"

#include <string>

namespace QaplaConfiguration {

QaplaHelpers::IniFile::Section toTournamentSection(
    const QaplaTester::TournamentConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "tournament";
    section.addEntry("id", id);
    section.addEntry("event", config.event);
    section.addEntry("type", config.type);
    section.addEntry("file", config.tournamentFilename);
    section.addEntry("saveintervalS", std::to_string(config.saveIntervalMs / 1000));
    section.addEntry("games", std::to_string(config.games));
    section.addEntry("rounds", std::to_string(config.rounds));
    section.addEntry("repeat", std::to_string(config.repeat));
    section.addEntry("ratinginterval", std::to_string(config.ratingInterval));
    section.addEntry("outcomeinterval", std::to_string(config.outcomeInterval));
    section.addEntry("averageelo", std::to_string(config.averageElo));
    section.addEntry("noswap", config.noSwap ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toOpeningsSection(
    const QaplaTester::Openings& openings, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "openings";
    section.addEntry("id", id);
    section.addEntry("file", openings.file);
    section.addEntry("order", openings.order);
    section.addEntry("plies", openings.plies ? std::to_string(*openings.plies + 1) : "all");
    section.addEntry("start", std::to_string(openings.start + 1));
    section.addEntry("srand", std::to_string(openings.seed));
    section.addEntry("policy", openings.policy);
    return section;
}

QaplaHelpers::IniFile::Section toPgnOutputSection(
    const QaplaTester::PgnSave::Options& options, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "pgnoutput";
    section.addEntry("id", id);
    section.addEntry("file", options.file);
    section.addEntry("append", options.append ? "true" : "false");
    section.addEntry("finished", options.onlyFinishedGames ? "true" : "false");
    section.addEntry("min", options.minimalTags ? "true" : "false");
    section.addEntry("clock", options.includeClock ? "true" : "false");
    section.addEntry("eval", options.includeEval ? "true" : "false");
    section.addEntry("depth", options.includeDepth ? "true" : "false");
    section.addEntry("pv", options.includePv ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toDrawAdjudicationSection(
    const QaplaTester::AdjudicationManager::DrawAdjudicationConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "draw";
    section.addEntry("id", id);
    section.addEntry("active", config.active ? "true" : "false");
    section.addEntry("movenumber", std::to_string(config.minFullMoves));
    section.addEntry("movecount", std::to_string(config.requiredConsecutiveMoves));
    section.addEntry("score", std::to_string(config.centipawnThreshold));
    section.addEntry("test", config.testOnly ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toResignAdjudicationSection(
    const QaplaTester::AdjudicationManager::ResignAdjudicationConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "resign";
    section.addEntry("id", id);
    section.addEntry("active", config.active ? "true" : "false");
    section.addEntry("movecount", std::to_string(config.requiredConsecutiveMoves));
    section.addEntry("score", std::to_string(config.centipawnThreshold));
    section.addEntry("twosided", config.twoSided ? "true" : "false");
    section.addEntry("test", config.testOnly ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toSprtSection(
    const QaplaTester::SprtConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "sprt";
    section.addEntry("id", id);
    section.addEntry("eloH0", std::to_string(config.eloH0));
    section.addEntry("eloH1", std::to_string(config.eloH1));
    section.addEntry("alpha", std::to_string(config.alpha));
    section.addEntry("beta", std::to_string(config.beta));
    section.addEntry("maxgames", std::to_string(config.maxGames));
    section.addEntry("model", config.model);
    section.addEntry("pentanomial", config.pentanomial ? "true" : "false");
    return section;
}

} // namespace QaplaConfiguration
