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

#include "imgui-engine-select.h"
#include "imgui-tournament-opening.h"
#include "imgui-engine-global-settings.h"

#include "qapla-tester/sprt-manager.h"

#include <memory>
#include <vector>
#include <cstdint>

namespace QaplaWindows {

    class SprtTournamentData {
    public:
        SprtTournamentData();
        ~SprtTournamentData();

        /**
         * @brief Returns the singleton instance of SprtTournamentData.
         * @return Reference to the SprtTournamentData singleton.
         */
        static SprtTournamentData& instance() {
            static SprtTournamentData instance;
            return instance;
        }

        /**
         * @brief Returns a reference to the engine selection.
         * @return Reference to the engine selection.
         */
        ImGuiEngineSelect& engineSelect() {
            return *engineSelect_;
        }

        /**
         * @brief Returns a const reference to the engine selection.
         * @return Const reference to the engine selection.
         */
        const ImGuiEngineSelect& engineSelect() const {
            return *engineSelect_;
        }

        /**
         * @brief Returns a reference to the tournament opening configuration.
         * @return Reference to the tournament opening configuration.
         */
        ImGuiTournamentOpening& tournamentOpening() {
            return *tournamentOpening_;
        }

        /**
         * @brief Returns a const reference to the tournament opening configuration.
         * @return Const reference to the tournament opening configuration.
         */
        const ImGuiTournamentOpening& tournamentOpening() const {
            return *tournamentOpening_;
        }

        /**
         * @brief Returns a reference to the global engine settings.
         * @return Reference to the global engine settings.
         */
        ImGuiEngineGlobalSettings& globalSettings() {
            return *globalSettings_;
        }

        /**
         * @brief Returns a const reference to the global engine settings.
         * @return Const reference to the global engine settings.
         */
        const ImGuiEngineGlobalSettings& globalSettings() const {
            return *globalSettings_;
        }

        /**
         * @brief Returns a reference to the SPRT configuration.
         * @return Reference to the SPRT configuration.
         */
        SprtConfig& sprtConfig() {
            return sprtConfig_;
        }

        /**
         * @brief Returns a const reference to the SPRT configuration.
         * @return Const reference to the SPRT configuration.
         */
        const SprtConfig& sprtConfig() const {
            return sprtConfig_;
        }

        /**
         * @brief Sets the engine configurations for the SPRT tournament.
         * @param configurations Vector containing all engine configurations.
         */
        void setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations);

        /**
         * @brief Updates the configuration in the Configuration singleton.
         * @details This method creates Section entries for all SPRT configuration aspects
         *          and stores them in the Configuration singleton using setSectionList.
         */
        void updateConfiguration();

    private:
        /**
         * @brief Sets up callbacks for UI component changes.
         * @details Registers callbacks for engine selection changes.
         */
        void setupCallbacks();

        /**
         * @brief Loads the engine selection configuration from the configuration file.
         * @details Retrieves engine selection settings from the "engineselection" section
         *          with ID "sprt-tournament" and applies them to the engine selector.
         */
        void loadEngineSelectionConfig();

        /**
         * @brief Loads the SPRT configuration from the configuration file.
         * @details Retrieves SPRT settings (elo bounds, alpha, beta, maxGames) from
         *          the "sprtconfig" section with ID "sprt-tournament".
         */
        void loadSprtConfig();

        /**
         * @brief Loads the global engine settings configuration from the configuration file.
         * @details Retrieves global engine and time control settings from the configuration
         *          sections with ID "sprt-tournament".
         */
        void loadGlobalSettingsConfig();

        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        std::unique_ptr<ImGuiTournamentOpening> tournamentOpening_;
        std::unique_ptr<ImGuiEngineGlobalSettings> globalSettings_;
        std::vector<ImGuiEngineSelect::EngineConfiguration> engineConfigurations_;

        SprtConfig sprtConfig_;
        ImGuiEngineGlobalSettings::GlobalSettings eachEngineConfig_;
        ImGuiEngineGlobalSettings::TimeControlSettings timeControlSettings_;
    };

}
