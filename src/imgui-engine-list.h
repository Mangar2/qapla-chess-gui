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

#include <game-manager/engine-record.h>
#include <base-elements/change-tracker.h>

#include <imgui.h>
#include <memory>
#include <vector>
#include <string>

namespace QaplaTester
{
    class GameRecord;
    struct MoveRecord;
    struct SearchInfo;
}

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class ImGuiEngineList  {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        ImGuiEngineList();
        ImGuiEngineList(ImGuiEngineList&&) noexcept;
        ImGuiEngineList& operator=(ImGuiEngineList&&) noexcept;
        virtual ~ImGuiEngineList();

        /**
         * @brief Renders the engine window and its components.
         * This method should be called within the main GUI rendering loop.
         * @return A pair containing the engine ID and command string if an action was triggered, otherwise empty strings.
         */
        virtual std::pair<std::string, std::string> draw();

        /**
         * @brief Sets whether user input is allowed in the engine list.
         * @param allow True to allow input, false to disallow.
         */
        void setAllowInput(bool allow) { 
            allowInput_ = allow; 
        }

        /**
         * @brief Sets the engine records for the list.
         * @param engineRecords The engine records to display.
         */
        void setEngineRecords(const QaplaTester::EngineRecords& engineRecords) {
            engineRecords_ = engineRecords;
        }   

        /**
         * @brief Gets the current engine records.
         * @return The current engine records.
         */
        const QaplaTester::EngineRecords& getEngineRecords() const {
            return engineRecords_;
        }

        /**
         * @brief Sets the move record for the list.
         * @param moveRecord The move record to display.
         * @param playerIndex The index of the player.
         * @param gameStatus The current status of the game.
         */
        void setFromMoveRecord(const QaplaTester::MoveRecord &moveRecord, uint32_t playerIndex,
                               const std::string &gameStatus = "");


        /**
         * @brief Sets the data from a GameRecord.
         * @param gameRecord The game record to extract data from.
         */
        void setFromGameRecord(const QaplaTester::GameRecord& gameRecord);

        /**
         * @brief Polls the log buffers for all engines and updates the log tables.
         */
        void pollLogBuffers();

    protected:
        /**
         * @brief Sets the log buffer for a specific player index.
         * @param logBuffer The ring buffer containing log messages.
         * @param playerIndex The index of the player.
         */
        void setFromLogBuffer(const QaplaTester::RingBuffer& logBuffer, uint32_t playerIndex);

    private:
        struct EngineInfoTable {
            std::unique_ptr<ImGuiTable> infoTable_;
            std::unique_ptr<ImGuiTable> logTable_;
            QaplaTester::ChangeTracker logTracker_{};
            bool showLog_ = false;
            size_t nextInputCount_ = 0;
        };

        /**
         * @brief Ensures that the number of tables matches the specified size.
         * @param size The desired number of tables.
         */
        void addTables(size_t size);

        /**
         * @brief Determines whether a move record should be displayed.
         *
         * @param moveRecord The move record to check.
         * @param playerIndex The index of the player (0 or 1).
         * @return true if the move record should be displayed, false otherwise.
         */
        bool shouldDisplayMoveRecord(const QaplaTester::MoveRecord &moveRecord, uint32_t playerIndex);

        /**
         * @brief Draws the engine space for a given index.
         * @param index Index of the engine to draw.
		 * @param size Size of the engine space.
         * @return String representing any action to perform (e.g., "stop", "restart") or empty if no action.
		 */
        std::string drawEngineSpace(size_t index, const ImVec2 size);


        std::string drawEngineArea(const ImVec2 &topLeft, ImDrawList *drawList, 
            const ImVec2 &max, float cEngineInfoWidth, size_t index, bool isSmall);

        std::string drawEngineTable(const ImVec2 &topLeft, float cEngineInfoWidth, 
            float cSectionSpacing, size_t index, const ImVec2 &max, const ImVec2 &size);

        void drawLog(const ImVec2 &topLeft, float cEngineInfoWidth, 
            float cSectionSpacing, size_t index, const ImVec2 &max, const ImVec2 &size);            

        void setInfoTable(size_t index, const QaplaTester::MoveRecord& moveRecord);

        std::vector<EngineInfoTable> infoTables_;
        std::vector<uint32_t> displayedMoveNo_;
        std::vector<uint32_t> infoCnt_;
        std::optional<size_t> nextHalfmoveNo_;
   
        QaplaTester::EngineRecords engineRecords_;
        QaplaTester::ChangeTracker gameRecordTracker_;

        bool allowInput_ = false;
    };

} // namespace QaplaWindows
