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
#include <memory>

namespace QaplaWindows
{

    /**
     * @brief Main window for managing the visual and interactive context of a board session.
     *
     * Hosts a single embedded layout or window that renders within this workspace.
     */
    class BoardWorkspace {
    public:
        /**
         * @brief Sets the root window to be displayed inside this workspace.
         * @param window The embedded window (e.g. board) to render.
         */
        void setRootWindow(std::unique_ptr<EmbeddedWindow> window);

        /**
         * @brief Renders the full workspace, including the root embedded window.
         */
        void draw();
        void maximize(bool maximized) { maximized_ = maximized; }

    private:
		void drawMaximized();
		bool maximized_ = false;
        std::unique_ptr<EmbeddedWindow> rootWindow;
    };
}
