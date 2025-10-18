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

    setupCallbacks();
    loadEngineSelectionConfig();
    tournamentOpening_->loadConfiguration();
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
