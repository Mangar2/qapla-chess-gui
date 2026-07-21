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
#include <string>

namespace QaplaTester::Settings {
    class Manager;
}

namespace QaplaConfiguration {

    /**
     * @brief Ensures QaplaTester::Settings::initSettings() has been called exactly once,
     * registering the shared configuration schema (tournament, sprt, openings, pgnoutput,
     * draw, resign, engine, ...) with QaplaTester::Settings::Manager.
     */
    void ensureSettingsRegistered();

    /**
     * @brief Parses a single group's sections into QaplaTester::Settings::Manager and
     * validates the group's completeness (filling in schema defaults).
     * @param sectionName The section/group name (e.g. "tournament", "openings", "sprt").
     * @param sections The sections to parse (typically just one, for this group).
     * @return Reference to the singleton Manager, ready for a matching *ConfigFile::fromManager() call.
     */
    QaplaTester::Settings::Manager& loadGroupIntoManager(
        const std::string& sectionName,
        const QaplaHelpers::IniFile::SectionList& sections);

    /**
     * @brief Convenience overload that pulls the section list for (sectionName, id) out of
     * the live Configuration singleton, then delegates to the SectionList overload.
     * @param sectionName The section/group name (e.g. "tournament", "openings", "sprt").
     * @param id The configuration id used for GUI multi-window scoping.
     * @return Reference to the singleton Manager, ready for a matching *ConfigFile::fromManager() call.
     */
    QaplaTester::Settings::Manager& loadGroupIntoManager(
        const std::string& sectionName,
        const std::string& id);

} // namespace QaplaConfiguration
