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
#include "callback-manager.h"
#include "qapla-tester/game-result.h"

using QaplaTester::GameRecord;
using QaplaTester::GameResult;

#include <fstream>
#include <sstream>
#include <set>

using namespace QaplaWindows;

ImGuiGameList::ImGuiGameList()
{
}

ImGuiGameList::~ImGuiGameList() {
    // Cancel any ongoing operation before joining
    OperationState currentState = operationState_.load();
    if (currentState == OperationState::Loading) {
        operationState_.store(OperationState::Cancelling);
    }
    
    if (loadingThread_.joinable()) {
        loadingThread_.join();
    }
}

void ImGuiGameList::draw() {
    drawButtons();
    drawLoadingStatus();
    ImGui::Indent(10.0F);
    drawGameTable();
    ImGui::Unindent(10.0F);
}

void ImGuiGameList::drawButtons() {
    constexpr float space = 3.0F;
    constexpr float topOffset = 5.0F;
    constexpr float bottomOffset = 8.0F;
    constexpr float leftOffset = 20.0F;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = {25.0F, 25.0F};

    const std::vector<std::string> buttons = {"Open", "Save As", "Filter"};
    const auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, buttons);
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    
    for (const std::string& button : buttons) {
        auto state = QaplaButton::ButtonState::Normal;
        std::string text = button;
        ImGui::SetCursorScreenPos(pos);
        if (button == "Open") {
            bool isLoading = operationState_.load() == OperationState::Loading;
            state = isLoading ? QaplaButton::ButtonState::Active : QaplaButton::ButtonState::Normal;
            text = isLoading ? "Stop" : "Open";
        } else {
            bool isLoading = operationState_.load() == OperationState::Loading;
            state = isLoading ? QaplaButton::ButtonState::Disabled : QaplaButton::ButtonState::Normal;
        }
        
        if (QaplaButton::drawIconButton(
                button, text, buttonSize, state,
                [&button, state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            if (button == "Open") {
                QaplaButton::drawOpen(drawList, topLeft, size, state);
            } else if (button == "Filter") {
                QaplaButton::drawFilter(drawList, topLeft, size, state);
            } else if (button == "Save As") {
                QaplaButton::drawSave(drawList, topLeft, size, state);
            }
        })) {
            // Handle button clicks
            bool isLoading = operationState_.load() == OperationState::Loading;
            if (button == "Open") {
                if (isLoading) {
                    // Stop loading
                    operationState_.store(OperationState::Cancelling);
                } else {
                    openFile();
                }
            } else if (!isLoading) {
                if (button == "Save As") {
                    SnackbarManager::instance().showNote("Save As button clicked - functionality not yet implemented");
                } else if (button == "Filter") {
                    SnackbarManager::instance().showNote("Filter button clicked - functionality not yet implemented");
                }
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void ImGuiGameList::drawLoadingStatus() {
    OperationState state = operationState_.load();
    if (state == OperationState::Idle) return;
    
    ImGui::Indent(10.0F);
    if (state == OperationState::Cancelling) {
        ImGui::Text("Cancelling loading from %s...", loadingFileName_.c_str());
    } else {
        ImGui::Text("Loading games from %s...", loadingFileName_.c_str());
    }
    float progress = loadingProgress_.load();
    ImGui::ProgressBar(progress, ImVec2(-10.0F, 20.0F), 
        std::to_string(gamesLoaded_.load()).c_str());
        //std::string(std::to_string(static_cast<int>(progress * 100.0F)) + "%").c_str());
    ImGui::Unindent(10.0F);
}

void ImGuiGameList::createTable() {
    const auto& games = gameRecordManager_.getGames();
    if (games.empty()) return;

    std::lock_guard<std::mutex> lock(gameTableMutex_);

    auto commonTagPairs = gameRecordManager_.getMostCommonTags(10); 
    std::vector<std::string> commonTags;
    for (const auto& pair : commonTagPairs) {
        commonTags.push_back(pair.first);
    }

    // Define fixed columns
    std::vector<ImGuiTable::ColumnDef> columns = {
        { .name = "White", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F },
        { .name = "Black", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F },
        { .name = "Result", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F },
        { .name = "PlyCount", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 65.0F, .alignRight = true }
    };

    auto knownTags = std::set<std::string>{};
    for (const auto& col : columns) {
        knownTags.insert(col.name);
    }

    // Add common tag columns
    for (const auto& tag : commonTags) {
        if (knownTags.find(tag) == knownTags.end()) { // Skip already included columns
            columns.push_back({tag, ImGuiTableColumnFlags_WidthFixed, 100.0F});
        }
    }

    // Initialize table with columns
    gameTable_ = ImGuiTable("GameListTable",
        ImGuiTableFlags_Borders | 
        ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY | 
        ImGuiTableFlags_ScrollX,
        columns);

    gameTable_.setClickable(true);
    gameTable_.setSortable(true);
    gameTable_.setFilterable(true);

    // Fill table with game data
    for (const auto& game : games) {
        const std::map<std::string, std::string>& tags = game.getTags();
        
        // Get fixed column data
        std::string white = tags.count("White") ? tags.at("White") : "";
        std::string black = tags.count("Black") ? tags.at("Black") : "";
        
        auto [cause, result] = game.getGameResult();
        std::string resultStr = gameResultToPgnResult(result);
        // Cause is not set in load games for speed reasons and cannot be used here
        
        std::string moves = std::to_string(game.history().size());
        
        std::vector<std::string> rowData = {white, black, resultStr, moves};
        
        for (const std::string& tag : commonTags) {
            // Only add if not already included
            if (knownTags.find(tag) == knownTags.end()) { 
                auto it = tags.find(tag);
                std::string tagValue = (it != tags.end()) ? it->second : "";
                rowData.push_back(tagValue);
            }
        }
        
        gameTable_.push(rowData);
    }
    gameTable_.setAutoScroll(true);

}

void ImGuiGameList::openFile() {
    auto selectedFiles = OsDialogs::openFileDialog(false);
    if (!selectedFiles.empty()) {
        // Wait for any previous thread to finish
        if (loadingThread_.joinable()) {
            loadingThread_.join();
        }
        
        // Start loading in background thread
        operationState_.store(OperationState::Loading);
        gamesLoaded_ = 0;
        loadingProgress_ = 0.0F;
        loadingFileName_ = selectedFiles[0];
        
        loadingThread_ = std::thread(&ImGuiGameList::loadFileInBackground, this, selectedFiles[0]);
    }
}

void ImGuiGameList::loadFileInBackground(const std::string& fileName) {
    try {
        gamesLoaded_ = 0;
        loadingProgress_ = 0.0F;

        gameRecordManager_.load(fileName, [&](const GameRecord&, float progress) {
            gamesLoaded_++;
            loadingProgress_ = progress;
            return operationState_.load() != OperationState::Cancelling; 
        });
        
        if (operationState_.load() == OperationState::Cancelling) {
            operationState_.store(OperationState::Idle);
            SnackbarManager::instance().showNote("Loading cancelled for " + fileName);
            return;
        }
        
        const auto& games = gameRecordManager_.getGames();
        gamesLoaded_ = games.size();
        
        // Create table with loaded data
        createTable();
        
        operationState_.store(OperationState::Idle);
        
        SnackbarManager::instance().showSuccess(
            "Loaded " + std::to_string(games.size()) + " games from " + fileName);
    } catch (const std::exception& e) {
        operationState_.store(OperationState::Idle);
        SnackbarManager::instance().showError("Failed to load file: " + std::string(e.what()));
    }
}

void ImGuiGameList::drawGameTable() {
    const auto& games = gameRecordManager_.getGames();
    if (games.empty()) return;

    // Use a child window to ensure proper scrolling
    auto availSize = ImGui::GetContentRegionAvail();
    
    // Only draw if we can acquire the lock without blocking
    if (gameTableMutex_.try_lock()) {
        std::lock_guard<std::mutex> lock(gameTableMutex_, std::adopt_lock);
        
        auto clickedIndex = gameTable_.draw(ImVec2(0, availSize.y)); 
        if (clickedIndex) {
            gameTable_.setCurrentRow(*clickedIndex);
            selectedGame_ = gameRecordManager_.loadGameByIndex(*clickedIndex);
        } else {
            selectedGame_ = std::nullopt;
        }
    }
}
