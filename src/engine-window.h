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

#include "embedded-window.h"
#include "imgui-popup.h"
#include "engine-setup-window.h"
#include <memory>

struct MoveRecord;
struct SearchInfo;

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class EngineWindow : public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        EngineWindow();
        ~EngineWindow();

        void draw() override;

    private:

        /**
         * @brief Shows a popup window to select engines using EngineSetupWindow.
         * @return Optional list of selected EngineConfig objects if user confirmed.
         */
        void drawEngineSelectionPopup();

        /**
         * @brief Draws the configuration button area for all engines.
         */
        void drawConfigButtonArea();
        
        std::unique_ptr<ImGuiPopup<EngineSetupWindow>> setupWindow_;

    };

} // namespace QaplaWindows
