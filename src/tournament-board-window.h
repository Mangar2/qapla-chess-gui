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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */
#pragma once


#include "qapla-engine/types.h"
#include "embedded-window.h"
#include "imgui-board.h"

class GameRecord;

namespace QaplaWindows
{

    class TournamentBoardWindow : public EmbeddedWindow
    {
    public:

        TournamentBoardWindow() = default;
        TournamentBoardWindow(TournamentBoardWindow&&) noexcept = default;
        TournamentBoardWindow& operator=(TournamentBoardWindow&&) noexcept = default;

        virtual ~TournamentBoardWindow() = default;

        /**
         * @brief Draw the window.
         */
        void draw() override;

        /**
         * @brief Set data from game record for the window.
         * @param gameRecord The game record to display.
         */
        void setFromGameRecord(const GameRecord& gameRecord);

        /**
         * @brief Set the active state of the window.
         * If inactive, the window is not updated with new data
         * @param active True to activate the window, false to deactivate it.
         */
        void setActive(bool active) { active_ = active; }

    private:
        bool active_ = false;
        ImGuiBoard imGuiBoard_;
    };

}
