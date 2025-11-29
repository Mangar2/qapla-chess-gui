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

#include "pgn-save.h"
#include "ini-file.h"

#include <string>
#include <vector>

class Tutorial;

namespace QaplaWindows {

    class ImGuiTournamentPgn {
    public:
        ImGuiTournamentPgn() = default;
        ~ImGuiTournamentPgn() = default;

        struct DrawOptions {
            float inputWidth = 150.0F;          ///< Width of input controls
            float fileInputWidth = 300.0F;      ///< Width of file input control
            float indent = 10.0F;               ///< Indentation for controls
            bool drawDetails = true;            ///< Whether to draw detailed options
            bool showCollapsingHeader = true;   ///< Whether to show collapsing header
        };

        /**
         * @brief Draws the PGN configuration UI controls.
         * @param options Drawing options for control widths and indentation
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if any configuration value was changed, false otherwise.
         */
        bool draw(const DrawOptions& options, 
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
        QaplaTester::PgnSave::Options& pgnOptions() {
            return pgnOptions_;
        }

        /**
         * @brief Returns a const reference to the PGN options.
         * @return Const reference to the PGN options.
         */
        [[nodiscard]] const QaplaTester::PgnSave::Options& pgnOptions() const {
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
        QaplaTester::PgnSave::Options pgnOptions_;
        std::string id_;
    };

}
