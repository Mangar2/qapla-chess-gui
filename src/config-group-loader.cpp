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

// NOTE: this file (and its declarations in config-group-loader.h) must stay free of
// any dependency on configuration.h / QaplaConfiguration::Configuration so it can be
// linked into the lightweight unit-tests target without pulling in the whole GUI.
// The Configuration-dependent convenience overload is defined in configuration.cpp instead.
#include "config-group-loader.h"

#include <cli/settings-definitions.h>
#include <cli/settings-manager.h>

#include <iostream>

namespace QaplaConfiguration {

void ensureSettingsRegistered() {
    static bool registered = []() {
        QaplaTester::Settings::initSettings();
        return true;
    }();
    (void)registered;
}

QaplaTester::Settings::Manager& loadGroupIntoManager(
    const std::string& sectionName,
    const QaplaHelpers::IniFile::SectionList& sections) {
    ensureSettingsRegistered();

    QaplaHelpers::ConfigData tmp;
    for (const auto& section : sections) {
        tmp.addSection(section);
    }

    auto& manager = QaplaTester::Settings::Manager::instance();
    // Clear any stale instances left over from a previous, unrelated call before
    // reparsing, so a group with zero sections this time round doesn't keep
    // returning a leftover instance from an earlier load.
    manager.clearGroup(sectionName);
    try {
        manager.parseInput(tmp, /*overwrite=*/true);
        manager.validateGroupCompleteness();
    } catch (const std::exception& e) {
        // Section content doesn't match the current schema (e.g. leftover data from an
        // older, incompatible file format). Fall back to an empty group rather than
        // crashing, so callers' *ConfigFile::fromManager() returns a default-constructed
        // config instead of propagating the exception.
        std::cerr << "Warning: failed to parse \"" << sectionName << "\" configuration, "
            << "using defaults instead: " << e.what() << "\n";
        manager.clearGroup(sectionName);
    }
    return manager;
}

} // namespace QaplaConfiguration
