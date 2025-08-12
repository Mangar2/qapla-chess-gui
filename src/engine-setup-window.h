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
#include "board-data.h"
#include <memory>

class EngineConfig;

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class EngineSetupWindow: public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        EngineSetupWindow(std::shared_ptr<BoardData> boardData);
        ~EngineSetupWindow();

        void draw() override;

    private:

        /**
         * @brief Draws a collapsible section for editing a single engine configuration.
         * @param config Reference to the engine configuration to edit.
         * @param index Index of the engine, used to generate unique ImGui IDs.
         */
        bool drawEngineConfigSection(EngineConfig& config, int index);

        std::shared_ptr<BoardData> boardData_;
    };

} // namespace QaplaWindows
