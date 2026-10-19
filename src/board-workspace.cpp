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
#include "board-workspace.h"
#include "snackbar.h"

#include "ui/qapla-ui.h"

#include <format>

namespace QaplaWindows {

    void BoardWorkspace::setRootWindow(std::unique_ptr<EmbeddedWindow> window) {
        rootWindow = std::move(window);
    }

    void BoardWorkspace::drawMaximized()
    {
    }

    void BoardWorkspace::draw() {
        try {
            QaplaUi::Vec2 pos = QaplaUi::getViewportPos();
            QaplaUi::Vec2 size = QaplaUi::getViewportSize();

            QaplaUi::pushStyleColor(QaplaUi::StyleColor::WindowBg, QaplaUi::Vec4(0, 0, 0, 0));
            QaplaUi::pushStyleVar(QaplaUi::StyleVar::WindowRounding, 0.0F);
            QaplaUi::pushStyleVar(QaplaUi::StyleVar::WindowBorderSize, 0.0F);
            QaplaUi::pushStyleVar(QaplaUi::StyleVar::WindowPadding, QaplaUi::Vec2(0, 0));

            QaplaUi::beginOverlay("RootOverlay", pos, size,
                QaplaUi::WindowFlags::NoTitleBar |
                QaplaUi::WindowFlags::NoResize |
                QaplaUi::WindowFlags::NoMove |
                QaplaUi::WindowFlags::NoCollapse |
                QaplaUi::WindowFlags::NoBringToFrontOnFocus |
                QaplaUi::WindowFlags::NoNavFocus);

            if (rootWindow) {
                if (QaplaUi::beginChild("root")) {
                    try {
                        rootWindow->draw();
                    }
                    catch (const std::exception& e) {
                        SnackbarManager::instance().showError(
                            std::format("Error in root window: {}", e.what()));
                    }
                    catch (...) {
                        SnackbarManager::instance().showError("Unknown error in root window");
                    }
                }
                QaplaUi::endChild();
            }

            QaplaUi::endOverlay();
            QaplaUi::popStyleVar(3);
            QaplaUi::popStyleColor(1);
        }
        catch (...) {
            QaplaUi::text("Error rendering Board Workspace");
        }
    }



} // namespace QaplaWindows
