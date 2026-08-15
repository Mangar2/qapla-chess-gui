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
#include <config/engine-global-config.h>
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
     * @brief Builds the "each" section of a tournament/SPRT state file from the GUI's global
     * engine settings.
     *
     * The GUI keeps every global setting together with a "use it or not" flag, so that a value
     * stays configured while it is switched off. A state file has no room for that distinction:
     * "each" is the CLI's defaults layer, and everything it lists takes effect. The two are
     * bridged by writing only the switched-on settings -- being listed *is* the use flag, which
     * is how fromEachSection() reads them back and how the CLI has always read them.
     *
     * Only keys of the CLI's "each" schema (settings-definitions.cpp, getEachKeys()) are written,
     * which is why the hash size goes out as the engine option "option.Hash" and the time control
     * as "tc" -- the GUI's own timecontroloptions section is not part of a state file.
     *
     * @param config The global engine settings, including the time control to apply.
     * @param id The state file's id, written as the section's "id" entry.
     * @return The "each" section, holding the switched-on settings only.
     */
    [[nodiscard]] QaplaHelpers::IniFile::Section toEachSection(
        const QaplaTester::EngineGlobalConfig& config, const std::string& id);

    /**
     * @brief Reads global engine settings from an "each" section.
     *
     * A setting counts as switched on when the section carries its value, matching what the CLI
     * makes of the same section and what toEachSection() writes. An explicit "use..." flag, as
     * written into qapla-chess-gui.ini so that a switched-off value survives a restart, still
     * wins over that rule; the legacy key "hash" is read like "option.Hash" for files written
     * before the CLI's spelling was adopted.
     *
     * @param section The "each" section to read.
     * @return The settings; timeControl is empty if the section names no time control.
     */
    [[nodiscard]] QaplaTester::EngineGlobalConfig fromEachSection(
        const QaplaHelpers::IniFile::Section& section);

    /**
     * @brief An "each" section and the engine sections it applies to, in the GUI's terms.
     */
    struct ResolvedEachDefaults {
        QaplaHelpers::IniFile::Section each;         ///< What is in force for every engine.
        QaplaHelpers::IniFile::SectionList engines;  ///< What each engine sets for itself.
    };

    /**
     * @brief Translates a state file's layering into the GUI's.
     *
     * The two tools layer engine settings the other way round. In a state file "each" holds the
     * defaults and an engine's own entry beats them; in the GUI a global setting in use beats
     * whatever the engine carries. Reading a file therefore takes two steps:
     *
     * 1. Apply the file's rule: every engine that does not set a key of its own inherits it from
     *    "each". This is a plain copy and covers every key, including the ones the GUI has no
     *    control for -- they survive in the engine entries instead of being dropped.
     * 2. Decide what "global" still means: a key all engines now agree on stays in "each" and is
     *    removed from them, which is the GUI's "in use". A key they disagree on cannot be global,
     *    so it stays with the engines; "each" keeps the value with an explicit "use...=false"
     *    where the GUI has a switch for it, so the user still sees what the file proposed.
     *
     * Inverse of withoutEachDefaults(), so that loading and saving a file leaves it unchanged.
     *
     * @param each The file's "each" section; carrying no more than an "id" means "no defaults".
     * @param engines The engine sections of the same state file.
     * @return The sections as the GUI understands them.
     */
    [[nodiscard]] ResolvedEachDefaults resolveEachDefaults(
        const QaplaHelpers::IniFile::Section& each,
        const QaplaHelpers::IniFile::SectionList& engines);

    /**
     * @brief Removes from the engine sections every key the "each" section already sets.
     *
     * A state file is read with the CLI's rule, where an engine's own entry beats "each". So a
     * global setting the GUI applies to all engines only survives if no engine section contradicts
     * it -- and engine sections contradict a lot: EngineConfig::toSection() writes "trace" and
     * "restart" for every engine, and "tc" for most. Without this step a global trace level could
     * never take effect outside the GUI.
     *
     * @param engines The engine sections to clean up.
     * @param each The "each" section holding the settings in force.
     * @return The engine sections, each key of "each" removed from all of them.
     */
    [[nodiscard]] QaplaHelpers::IniFile::SectionList withoutEachDefaults(
        const QaplaHelpers::IniFile::SectionList& engines,
        const QaplaHelpers::IniFile::Section& each);

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
