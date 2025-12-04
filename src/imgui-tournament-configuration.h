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

#include "tutorial.h"
#include "ini-file.h"

#include <string>
#include <cstdint>
#include <vector>

namespace QaplaTester {
    struct TournamentConfig;
}

namespace QaplaWindows {

    /**
     * @brief ImGui component for rendering tournament configuration settings.
     * 
     * This class handles the UI for tournament-specific settings like event name,
     * tournament type, rounds, games per pairing, etc.
     */
    class ImGuiTournamentConfiguration {
    public:
        /**
         * @brief Options to control which UI elements are displayed.
         */
        struct DrawOptions {
            bool alwaysOpen = false;         ///< Whether the configuration panel is always open
            bool showEvent = true;           ///< Show event name input
            bool showType = true;            ///< Show tournament type selection
            bool showRounds = true;          ///< Show rounds input
            bool showGamesPerPairing = true; ///< Show games per pairing input
            bool showSameOpening = true;     ///< Show same opening (repeat) input
            bool showNoColorSwap = true;     ///< Show no color swap checkbox
            bool showAverageElo = true;      ///< Show average Elo input
        };

        ImGuiTournamentConfiguration() = default;
        ~ImGuiTournamentConfiguration() = default;

        /**
         * @brief Renders the tournament configuration UI.
         * @param options Options controlling which elements to display.
         * @param inputWidth Width for input controls.
         * @param indent Indentation level.
         * @param tutorialContext Tutorial context with highlighting and annotations.
         * @return True if any value was changed, false otherwise.
         */
        bool draw(const DrawOptions& options, float inputWidth, float indent,
            const Tutorial::TutorialContext& tutorialContext = Tutorial::TutorialContext{});

        /**
         * @brief Sets the ID for configuration storage.
         * @param id The configuration ID.
         */
        void setId(const std::string& id) {
            id_ = id;
        }

        /**
         * @brief Returns a reference to the tournament configuration.
         * @return Reference to the tournament configuration.
         */
        QaplaTester::TournamentConfig& config() {
            return *config_;
        }

        /**
         * @brief Returns a const reference to the tournament configuration.
         * @return Const reference to the tournament configuration.
         */
        [[nodiscard]] const QaplaTester::TournamentConfig& config() const {
            return *config_;
        }

        /**
         * @brief Sets the tournament configuration pointer.
         * @param config Pointer to the tournament configuration to use.
         */
        void setConfig(QaplaTester::TournamentConfig* config) {
            config_ = config;
        }

        /**
         * @brief Loads configuration from the Configuration singleton.
         */
        void loadConfiguration();

        /**
         * @brief Returns the configuration sections for saving.
         * @return Vector of IniFile sections.
         */
        [[nodiscard]] std::vector<QaplaHelpers::IniFile::Section> getSections() const;

    private:
        /**
         * @brief Updates the configuration in the Configuration singleton.
         */
        void updateConfiguration() const;

        QaplaTester::TournamentConfig* config_ = nullptr;
        std::string id_;
    };

}
