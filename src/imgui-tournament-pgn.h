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

#include "qapla-tester/pgn-io.h"
#include "qapla-tester/ini-file.h"

#include <string>
#include <vector>

class Tutorial;

namespace QaplaWindows {

    class ImGuiTournamentPgn {
    public:
        ImGuiTournamentPgn() = default;
        ~ImGuiTournamentPgn() = default;

        /**
         * @brief Draws the PGN configuration UI controls.
         * @param inputWidth Width of the input controls.
         * @param fileInputWidth Width of the file input control.
         * @param indent Indentation to apply to the controls.
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if any configuration value was changed, false otherwise.
         */
        bool draw(float inputWidth, float fileInputWidth, float indent, 
            const Tutorial::TutorialContext& tutorialContext = Tutorial::TutorialContext{});

        /**
         * @brief Sets the ID for this PGN configuration.
         * @param id The ID to use for configuration storage.
         */
        void setId(const std::string& id) {
            id_ = id;
        }

        /**
         * @brief Returns a reference to the PGN options.
         * @return Reference to the PGN options.
         */
        QaplaTester::PgnIO::Options& pgnOptions() {
            return pgnOptions_;
        }

        /**
         * @brief Returns a const reference to the PGN options.
         * @return Const reference to the PGN options.
         */
        [[nodiscard]] const QaplaTester::PgnIO::Options& pgnOptions() const {
            return pgnOptions_;
        }

        /**
         * @brief Loads the PGN configuration from the configuration file.
         */
        void loadConfiguration();

        /**
         * @brief Gets the configuration sections for saving.
         * @return Vector of INI file sections containing the PGN configuration.
         */
        [[nodiscard]] std::vector<QaplaHelpers::IniFile::Section> getSections() const;

    private:
        QaplaTester::PgnIO::Options pgnOptions_;
        std::string id_;
    };

}
