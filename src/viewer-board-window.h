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


#include "qapla-engine/types.h"

#include "engine-record.h"

#include "embedded-window.h"
#include "imgui-board.h"
#include "imgui-engine-list.h"
#include "imgui-clock.h"
#include "imgui-move-list.h"
#include "imgui-barchart.h"

namespace QaplaTester
{
    struct MoveRecord;
    class GameRecord;
}

namespace QaplaWindows
{

    class VerticalSplitContainer;
    class HorizontalSplitContainer;

    class ViewerBoardWindow : public EmbeddedWindow
    {
    public:

        ViewerBoardWindow();
        ViewerBoardWindow(ViewerBoardWindow&&) noexcept;
        ViewerBoardWindow& operator=(ViewerBoardWindow&&) noexcept;

        ~ViewerBoardWindow() override;

        /**
         * @brief Draw the window.
         */
        void draw() override;

        /**
         * @brief Set data from game record for the window.
         * @param gameRecord The game record to display.
         */
        void setFromGameRecord(const QaplaTester::GameRecord& gameRecord);

        /**
         * @brief Set data from engine records for the window.
         * @param engineRecords The engine records to display.
         */
        void setFromEngineRecords(const QaplaTester::EngineRecords& engineRecords);

        /**
         * @brief Set data from move record for the window.
         * @param moveRecord The move record to display.
         * @param playerIndex The index of the player, 0 is always white, 1 always black.
         */
        void setFromMoveRecord(const QaplaTester::MoveRecord& moveRecord, uint32_t playerIndex);

        /**
         * @brief Set the active state of the window.
         * If inactive, the window is not updated with new data
         * @param active True to activate the window, false to deactivate it.
         */
        void setActive(bool active) { active_ = active; }
        [[nodiscard]] bool isActive() const { return active_; }

        /**
         * @brief Set the running state of the window.
         * @param running True if the game shown in the window is running, 
         * false otherwise.
         */
        void setRunning(bool running) { running_ = running; }
        [[nodiscard]] bool isRunning() const { return running_; }

        /**
         * Get the unique identifier "round.game" for this board instance.
         * @return The unique identifier as a string.
         */
        [[nodiscard]] std::string id() const;

    private:
        bool active_ = false;
        bool running_ = false;
        
        uint32_t round_ = 0;
        uint32_t gameInRound_ = 0;
        std::string positionName_;

        static VerticalSplitContainer& getMainWindow();
        static HorizontalSplitContainer& getTopWindow();
        static VerticalSplitContainer& getClockMovesWindow();
        static VerticalSplitContainer& getMovesChartWindow();

        ImGuiEngineList imGuiEngineList_;
        ImGuiBoard imGuiBoard_;
        ImGuiClock imGuiClock_;
        ImGuiMoveList imGuiMoveList_;
        ImGuiBarChart imGuiBarChart_;
    };

}
