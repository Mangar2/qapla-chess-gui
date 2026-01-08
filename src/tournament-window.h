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

#include "embedded-window.h"
#include "imgui-engine-select.h"
#include "tutorial.h"

#include "engine-handling/engine-config.h"

#include <memory>

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class TournamentWindow: public EmbeddedWindow {
    public:
        TournamentWindow() = default;
        ~TournamentWindow() = default;

        void draw() override;
        
        /**
         * @brief Indicates whether the Tournament tab should be highlighted for tutorial.
         * @return true if tutorial is active and not completed
         */
        bool highlighted() const override;

        /**
         * @brief Advances the Tournament tutorial based on user actions.
         * @param clickedButton The button that was clicked (empty string for state checks)
         */
        struct HighlightInfo {
            std::string highlightedButton = {};
            std::string highlightedSection = {};
            Tutorial::TutorialContext globalSettingsTutorial = {};
            Tutorial::TutorialContext openingTutorial = {};
            Tutorial::TutorialContext tournamentTutorial = {};
            Tutorial::TutorialContext timeControlTutorial = {};
            Tutorial::TutorialContext pgnTutorial = {};
        };

        static void showNextTournamentTutorialStep(const std::string& clickedButton);
        static void clearTournamentTutorialState();
        static void applyHighlighting(const HighlightInfo& info);

        static inline uint32_t tutorialProgress_ = 0; ///< Progress counter for Tournament tutorial
        static inline std::string highlightedButton_ = ""; ///< Button to highlight for tutorial
        static inline std::string highlightedSection_ = ""; ///< Section to highlight for tutorial
        static inline Tutorial::TutorialContext globalSettingsTutorial_; ///< Tutorial context for Global Engine Settings
        static inline Tutorial::TutorialContext openingTutorial_; ///< Tutorial context for Opening section
        static inline Tutorial::TutorialContext tournamentTutorial_; ///< Tutorial context for Tournament section
        static inline Tutorial::TutorialContext timeControlTutorial_; ///< Tutorial context for Time Control section
        static inline Tutorial::TutorialContext pgnTutorial_; ///< Tutorial context for PGN section

    private:

        static std::string drawButtons();
        static void executeCommand(const std::string &button);
        static bool drawInput();
        static void drawProgress();
        
        /**
         * @brief Checks if two engines with same originalName are selected and one has ponder enabled
         * @return true if tutorial condition is met
         */
        static bool hasTwoSameEnginesWithPonder();
    };

} // namespace QaplaWindows
