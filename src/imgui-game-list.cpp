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

#include "imgui-game-list.h"
#include "imgui-button.h"
#include "snackbar.h"
#include "os-dialogs.h"
#include <fstream>
#include <sstream>

using namespace QaplaWindows;

ImGuiGameList::~ImGuiGameList() {
    if (loadingThread_.joinable()) {
        loadingThread_.join();
    }
}

void ImGuiGameList::draw() {
    drawButtons();
    drawLoadingStatus();
}

void ImGuiGameList::drawButtons() {
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = {25.0f, 25.0f};

    const std::vector<std::string> buttons = {"Open", "Save", "Save As", "Filter"};
    const auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, buttons);
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    
    for (const std::string& button : buttons) {
        ImGui::SetCursorScreenPos(pos);
        auto state = isLoading_ ? QaplaButton::ButtonState::Disabled : QaplaButton::ButtonState::Normal;
        
        if (QaplaButton::drawIconButton(
                button, button, buttonSize, state, 
                [&button, state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            if (button == "Save") {
                QaplaButton::drawSave(drawList, topLeft, size, state);
            } else if (button == "Open") {
                QaplaButton::drawOpen(drawList, topLeft, size, state);
            }
            // TODO: Implement icons for Save As, Filter
        })) {
            // Handle button clicks - only if not loading
            if (!isLoading_) {
                if (button == "Save") {
                    SnackbarManager::instance().showNote("Save button clicked - functionality not yet implemented");
                } else if (button == "Save As") {
                    // TODO: Implement save as functionality
                } else if (button == "Open") {
                    openFile();
                } else if (button == "Filter") {
                    // TODO: Implement filter functionality
                }
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void ImGuiGameList::drawLoadingStatus() {
    if (!isLoading_) return;
    ImGui::Indent(10.0f);
    ImGui::Text("Loading games from %s...", loadingFileName_.c_str());
    ImGui::Text("Games loaded: %zu", gamesLoaded_.load());
    ImGui::Unindent(10.0f);
}

void ImGuiGameList::openFile() {
    auto selectedFiles = OsDialogs::openFileDialog(false);
    if (!selectedFiles.empty()) {
        // Start loading in background thread
        isLoading_ = true;
        gamesLoaded_ = 0;
        loadingFileName_ = selectedFiles[0];
        
        loadingThread_ = std::thread(&ImGuiGameList::loadFileInBackground, this, selectedFiles[0]);
    }
}

void ImGuiGameList::loadFileInBackground(const std::string& fileName) {
    try {
        // Clear existing games
        gameRecordManager_.load(fileName);
        
        // For progress indication, we'll simulate loading by counting games
        const auto& games = gameRecordManager_.getGames();
        gamesLoaded_ = games.size();
        
        // Reset loading state
        isLoading_ = false;
        
        // Show success message
        SnackbarManager::instance().showSuccess(
            "Loaded " + std::to_string(games.size()) + " games from " + fileName);
    } catch (const std::exception& e) {
        isLoading_ = false;
        SnackbarManager::instance().showError("Failed to load file: " + std::string(e.what()));
    }
}