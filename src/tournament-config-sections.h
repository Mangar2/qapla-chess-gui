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
#include <opening/openings.h>
#include <opening/pgn-save.h>
#include <game-manager/adjudication-manager.h>
#include <sprt/sprt-manager.h>

#include <string>

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

} // namespace QaplaConfiguration
