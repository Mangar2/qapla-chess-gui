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

#include "qapla-tester/engine-config.h"

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

    private:
        static void drawButtons();
        static void executeCommand(const std::string &button);
        static bool drawInput();
        static void drawProgress();
    };

} // namespace QaplaWindows
