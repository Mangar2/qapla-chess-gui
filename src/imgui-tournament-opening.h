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

#include "opening/openings.h"
#include "base-elements/ini-file.h"

#include <string>
#include <optional>
#include <cstdint>
#include <vector>

class Tutorial;

namespace QaplaWindows {

    /**
     * @brief Parameters for drawing the tournament opening UI controls.
     */
    struct TournamentOpeningDrawParams {
        float inputWidth = 150.0F;           ///< Width for input controls
        float fileInputWidth = 300.0F;       ///< Width for file input control
        float indent = 10.0F;                ///< Indentation level

        bool showOpeningFile = true;         ///< Show opening file input
        bool showOrder = true;               ///< Show order selection
        bool showPlies = true;               ///< Show plies input
        bool showFirstOpening = true;        ///< Show first opening input
        bool showRandomSeed = true;          ///< Show random seed input
        bool showSwitchPolicy = true;        ///< Show switch policy selection
    };

    class ImGuiTournamentOpening {
    public:
        ImGuiTournamentOpening() = default;
        ~ImGuiTournamentOpening() = default;

        /**
         * @brief Parameters for drawing the tournament opening UI controls.
         */
        struct DrawParams {
            float inputWidth = 150.0F;           ///< Width for input controls
            float fileInputWidth = 300.0F;       ///< Width for file input control
            float indent = 10.0F;                ///< Indentation level

            bool alwaysOpen = false;             ///< Whether the section is always open
            bool showOpeningFile = true;         ///< Show opening file input
            bool showOrder = true;               ///< Show order selection
            bool showPlies = true;               ///< Show plies input
            bool showFirstOpening = true;        ///< Show first opening input
            bool showRandomSeed = true;          ///< Show random seed input
            bool showSwitchPolicy = true;        ///< Show switch policy selection
        };

        /**
         * @brief Renders the tournament opening configuration UI.
         * @param params Draw parameters including control visibility flags
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if any value was changed, false otherwise.
         */
        bool draw(const DrawParams& params, 
            const Tutorial::TutorialContext& tutorialContext = Tutorial::TutorialContext{});

        void setId(const std::string& id) {
            id_ = id;
        }

        QaplaTester::Openings& openings() {
            return openings_;
        }

        [[nodiscard]] const QaplaTester::Openings& openings() const {
            return openings_;
        }

        void loadConfiguration();
        [[nodiscard]] std::vector<QaplaHelpers::IniFile::Section> getSections() const;

    private:
        /**
         * @brief Updates the configuration in the Configuration singleton.
         */
        void updateConfiguration() const;

        /**
         * @brief Draws the opening file input control.
         * @param fileInputWidth Width for file input control
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if the value was changed, false otherwise.
         */
        bool drawOpeningFile(float fileInputWidth, const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the order selection control.
         * @param inputWidth Width for input control
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if the value was changed, false otherwise.
         */
        bool drawOrder(float inputWidth, const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the plies input control.
         * @param inputWidth Width for input control
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if the value was changed, false otherwise.
         */
        bool drawPlies(float inputWidth, const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the first opening input control.
         * @param inputWidth Width for input control
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if the value was changed, false otherwise.
         */
        bool drawFirstOpening(float inputWidth, const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the random seed input control.
         * @param inputWidth Width for input control
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if the value was changed, false otherwise.
         */
        bool drawRandomSeed(float inputWidth, const Tutorial::TutorialContext& tutorialContext);

        /**
         * @brief Draws the switch policy selection control.
         * @param inputWidth Width for input control
         * @param tutorialContext Tutorial context with highlighting and annotations
         * @return True if the value was changed, false otherwise.
         */
        bool drawSwitchPolicy(float inputWidth, const Tutorial::TutorialContext& tutorialContext);

        QaplaTester::Openings openings_;
        std::string id_;
    };

}
