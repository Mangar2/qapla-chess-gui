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

#include "sprt-tournament-data.h"
#include "configuration.h"

#include "qapla-tester/ini-file.h"
#include "qapla-tester/string-helper.h"

#include <algorithm>

using namespace QaplaWindows;

SprtTournamentData::SprtTournamentData() : 
    engineSelect_(std::make_unique<ImGuiEngineSelect>()),
    tournamentOpening_(std::make_unique<ImGuiTournamentOpening>())
{
    ImGuiEngineSelect::Options options;
    options.allowGauntletEdit = true;
    options.allowPonderEdit = true;
    options.allowTimeControlEdit = true;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = true;
    options.allowMultipleSelection = true;
    engineSelect_->setOptions(options);

    tournamentOpening_->setId("sprt-tournament");

    sprtConfig_.eloLower = -5;
    sprtConfig_.eloUpper = 5;
    sprtConfig_.alpha = 0.05;
    sprtConfig_.beta = 0.05;
    sprtConfig_.maxGames = 100000;

    setupCallbacks();
    loadEngineSelectionConfig();
    tournamentOpening_->loadConfiguration();
    loadSprtConfig();
}

SprtTournamentData::~SprtTournamentData() = default;

void SprtTournamentData::setupCallbacks() {
    engineSelect_->setConfigurationChangedCallback(
        [this](const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
            engineConfigurations_ = configurations;
        }
    );
}

void SprtTournamentData::setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
    engineConfigurations_ = configurations;
}

void SprtTournamentData::loadEngineSelectionConfig() {
    auto sections = QaplaConfiguration::Configuration::instance()
        .getConfigData().getSectionList("engineselection", "sprt-tournament")
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    engineSelect_->setId("sprt-tournament");
    engineSelect_->setEngineConfiguration(sections);
}

void SprtTournamentData::loadSprtConfig() {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto sections = configData.getSectionList("sprtconfig", "sprt-tournament");
    if (!sections || sections->empty()) {
        return;
    }

    for (const auto& [key, value] : (*sections)[0].entries) {
        if (key == "eloLower") {
            sprtConfig_.eloLower = QaplaHelpers::to_int(value).value_or(-5);
            sprtConfig_.eloLower = std::clamp(sprtConfig_.eloLower, -1000, 1000);
        }
        else if (key == "eloUpper") {
            sprtConfig_.eloUpper = QaplaHelpers::to_int(value).value_or(5);
            sprtConfig_.eloUpper = std::clamp(sprtConfig_.eloUpper, -1000, 1000);
        }
        else if (key == "alpha") {
            try {
                sprtConfig_.alpha = std::stod(value);
                sprtConfig_.alpha = std::clamp(sprtConfig_.alpha, 0.001, 0.5);
            } catch (...) {
                sprtConfig_.alpha = 0.05;
            }
        }
        else if (key == "beta") {
            try {
                sprtConfig_.beta = std::stod(value);
                sprtConfig_.beta = std::clamp(sprtConfig_.beta, 0.001, 0.5);
            } catch (...) {
                sprtConfig_.beta = 0.05;
            }
        }
        else if (key == "maxGames") {
            sprtConfig_.maxGames = QaplaHelpers::to_uint32(value).value_or(100000);
        }
    }
}

void SprtTournamentData::updateConfiguration() {
    QaplaHelpers::IniFile::KeyValueMap sprtEntries{
        {"id", "sprt-tournament"},
        {"eloLower", std::to_string(sprtConfig_.eloLower)},
        {"eloUpper", std::to_string(sprtConfig_.eloUpper)},
        {"alpha", std::to_string(sprtConfig_.alpha)},
        {"beta", std::to_string(sprtConfig_.beta)},
        {"maxGames", std::to_string(sprtConfig_.maxGames)}
    };

    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "sprtconfig", "sprt-tournament", {{
            .name = "sprtconfig",
            .entries = sprtEntries
        }});

    auto openingSections = tournamentOpening_->getSections();
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "opening", "sprt-tournament", openingSections);
}
