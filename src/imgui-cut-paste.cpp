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

#include "imgui-cut-paste.h"

#include "ui/qapla-ui.h"

namespace QaplaWindows::ImGuiCutPaste {

std::optional<std::string> getClipboardString() {
    auto text = QaplaUi::getClipboardText();
    if (text.empty()) {
        return std::nullopt;
    }

    return text;
}

bool setClipboardString(const std::string& text) {
    QaplaUi::setClipboardText(text.c_str());
    return true;
}

bool checkForCopy() {

    // Check for platform-specific copy shortcuts
    // Windows/Linux: Ctrl+C, macOS: Cmd+C (Super key)
    bool ctrlOrCmd = QaplaUi::isKeyDown(QaplaUi::Key::LeftCtrl) || 
                     QaplaUi::isKeyDown(QaplaUi::Key::RightCtrl) ||
                     QaplaUi::isKeyDown(QaplaUi::Key::LeftSuper) || 
                     QaplaUi::isKeyDown(QaplaUi::Key::RightSuper);

    return ctrlOrCmd && QaplaUi::isKeyPressed(QaplaUi::Key::C);
}

std::optional<std::string> checkForPaste() {
    // Check for platform-specific paste shortcuts
    // Windows/Linux: Ctrl+V, macOS: Cmd+V (Super key)
    bool ctrlOrCmd = QaplaUi::isKeyDown(QaplaUi::Key::LeftCtrl) || 
                     QaplaUi::isKeyDown(QaplaUi::Key::RightCtrl) ||
                     QaplaUi::isKeyDown(QaplaUi::Key::LeftSuper) || 
                     QaplaUi::isKeyDown(QaplaUi::Key::RightSuper);

    if (ctrlOrCmd && QaplaUi::isKeyPressed(QaplaUi::Key::V)) {
        return getClipboardString();
    }

    return std::nullopt;
}

} // namespace QaplaWindows::ImGuiCutPaste