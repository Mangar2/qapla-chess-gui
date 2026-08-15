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

#pragma once

#include <base-elements/ini-file.h>
#include <tournament/tournament-config.h>
#include <engine-handling/engine-config.h>
#include <opening/openings.h>
#include <opening/pgn-save.h>
#include <game-manager/adjudication-manager.h>
#include <sprt/sprt-manager.h>

#include <string>
#include <vector>

/**
 * @brief Free functions that build the shared-schema INI sections ("tournament",
 * "openings", "pgnoutput", "draw", "resign", "sprt") from the GUI's in-memory
 * configuration structs. These are the write-side counterpart to the library's
 * *ConfigFile::fromManager() factories, using the exact key names from
 * extern/qapla-engine-tester/src/cli/settings-definitions.cpp so that files
 * written by the GUI are byte-compatible with files written by the CLI/MCP.
 *
 * Kept free of any ImGui dependency so they can be exercised directly by unit tests.
 */
namespace QaplaConfiguration {

    [[nodiscard]] QaplaHelpers::IniFile::Section toTournamentSection(
        const QaplaTester::TournamentConfig& config, const std::string& id);

    [[nodiscard]] QaplaHelpers::IniFile::Section toOpeningsSection(
        const QaplaTester::Openings& openings, const std::string& id);

    [[nodiscard]] QaplaHelpers::IniFile::Section toPgnOutputSection(
        const QaplaTester::PgnSave::Options& options, const std::string& id);

    [[nodiscard]] QaplaHelpers::IniFile::Section toDrawAdjudicationSection(
        const QaplaTester::AdjudicationManager::DrawAdjudicationConfig& config, const std::string& id);

    [[nodiscard]] QaplaHelpers::IniFile::Section toResignAdjudicationSection(
        const QaplaTester::AdjudicationManager::ResignAdjudicationConfig& config, const std::string& id);

    [[nodiscard]] QaplaHelpers::IniFile::Section toSprtSection(
        const QaplaTester::SprtConfig& config, const std::string& id);

    /**
     * @brief Builds the "engine" sections of a tournament/SPRT state file from an engine
     * selection list: one section per participant, none for the rest.
     *
     * The GUI's selection list also holds the engines the user deselected, so that their
     * settings survive until they are added back. That is state for qapla-chess-gui.ini
     * only. A state file names the entrants: everything outside the GUI -- CLI and MCP --
     * enters every "engine" section it finds into the tournament, so a deselected engine
     * written here would silently play. For the same reason the GUI-only "selected" key is
     * dropped rather than written as "true".
     *
     * @param engines The selection list, selected and deselected entries mixed.
     * @param id The state file's id, written as the sections' "id" entry.
     * @return One section per selected engine, in list order.
     */
    [[nodiscard]] QaplaHelpers::IniFile::SectionList toParticipantSections(
        const std::vector<QaplaTester::EngineConfig>& engines, const std::string& id);

} // namespace QaplaConfiguration
