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
#include "imgui-controls.h"
#include "snackbar.h"
#include "os-dialogs.h"
#include "callback-manager.h"

#include <pgn-io.h>
#include <timer.h>
#include <string-helper.h>

#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <format>
#include <filesystem>
#include <system_error>

using QaplaTester::GameRecord;
using QaplaTester::GameResult;
using QaplaTester::PgnIO;

using namespace QaplaWindows;

ImGuiGameList::ImGuiGameList()
    : filterPopup_(
        ImGuiPopup<GameFilterWindow>::Config{
            .title = "Filter Games",
            .okButton = true,
            .cancelButton = true
        },
        ImVec2(550, 700)  // Increased height for better fit
    )
{
    init();
}

void ImGuiGameList::init() {
    filterPopup_.content().init("gamelist");
    filterPopup_.content().setOnFilterChangedCallback([this]() {
        updateFilterConfiguration();
    });
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
    
    // Draw filter popup if open
    filterPopup_.draw();
    
    // Check if filter was confirmed or cancelled
    auto confirmed = filterPopup_.confirmed();
    if (confirmed.has_value()) {
        if (*confirmed) {
            // OK was clicked - apply filter and recreate table
            updateFilterConfiguration();
            createTable();
        }
        filterPopup_.resetConfirmation();
    }
    
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

    const std::vector<std::string> buttons = {"Open", "Recent", "Save As", "Filter"};
    const auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, buttons);
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    
    for (const std::string& button : buttons) {
        bool isLoading = operationState_.load() == OperationState::Loading;
        auto [state, text] = computeButtonState(button, isLoading);
        ImGui::SetCursorScreenPos(pos);
        
        if (QaplaButton::drawIconButton(
                button, text, buttonSize, state,
                [&button, state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
            if (button == "Open") {
                QaplaButton::drawOpen(drawList, topLeft, size, state);
                ImGuiControls::hooverTooltip(state == QaplaButton::ButtonState::Active 
                    ? "Stop loading PGN file" 
                    : "Open PGN file to load games");
            } else if (button == "Recent") {
                QaplaButton::drawOpen(drawList, topLeft, size, state);
                ImGuiControls::hooverTooltip("Load recently auto-saved PGN file");
            } else if (button == "Filter") {
                QaplaButton::drawFilter(drawList, topLeft, size, state);
                ImGuiControls::hooverTooltip("Open filter dialog to filter games by criteria");
            } else if (button == "Save As") {
                QaplaButton::drawSave(drawList, topLeft, size, state);
                ImGuiControls::hooverTooltip("Save filtered games to new PGN file");
            }
        })) {
            executeCommand(button, isLoading);
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

std::pair<QaplaButton::ButtonState, std::string> ImGuiGameList::computeButtonState(const std::string& button, bool isLoading) const {
    auto state = QaplaButton::ButtonState::Normal;
    std::string text = button;
    if (button == "Open") {
        state = isLoading ? QaplaButton::ButtonState::Active : QaplaButton::ButtonState::Normal;
        text = isLoading ? "Stop" : "Open";
    } else if (button == "Recent") {
        state = isLoading ? QaplaButton::ButtonState::Disabled : QaplaButton::ButtonState::Normal;
        text = "Recent";
    } else if (button == "Filter") {
        const auto& filterData = filterPopup_.content().getFilterData();
        bool filterActive = filterData.hasActiveFilters();
        if (isLoading) {
            state = QaplaButton::ButtonState::Disabled;
        } else if (filterActive) {
            state = QaplaButton::ButtonState::Active;
        } else {
            state = QaplaButton::ButtonState::Normal;
        }
    } else {
        state = isLoading ? QaplaButton::ButtonState::Disabled : QaplaButton::ButtonState::Normal;
    }
    return {state, text};
}

void ImGuiGameList::executeCommand(const std::string& button, bool isLoading) {
    if (button == "Open") {
        if (isLoading) {
            // Inform the loading thread to cancel 
            operationState_.store(OperationState::Cancelling);
        } else {
            openFile();
        }
    } else if (!isLoading) {
        if (button == "Recent") {
            loadFileInBackground(PgnAutoSaver::instance().getFilePath());
        } else if (button == "Save As") {
            saveAsFile();
        } else if (button == "Filter") {
            updateFilterOptions();
            filterPopup_.open();
        }
    }
}

void ImGuiGameList::drawLoadingStatus() {
    OperationState state = operationState_.load();
    if (state == OperationState::Idle) {
        return;
    }
    
    ImGui::Indent(10.0F);
    if (state == OperationState::Cancelling) {
        if (!savingFileName_.empty()) {
            ImGui::Text("Cancelling saving to %s...", savingFileName_.c_str());
        } else {
            ImGui::Text("Cancelling loading from %s...", loadingFileName_.c_str());
        }
    } else if (state == OperationState::Saving) {
        ImGui::Text("Saving games to %s...", savingFileName_.c_str());
    } else {
        ImGui::Text("Loading games from %s...", loadingFileName_.c_str());
    }
    float progress = loadingProgress_.load();
    ImGui::ProgressBar(progress, ImVec2(-10.0F, 20.0F), 
        std::to_string(gamesLoaded_.load()).c_str());
        //std::string(std::to_string(static_cast<int>(progress * 100.0F)) + "%").c_str());
    ImGui::Unindent(10.0F);
}

static std::vector<std::string> createTableRow(const QaplaTester::GameRecord& game,
                                                        const std::vector<std::string>& commonTags,
                                                        const std::set<std::string>& knownTags)  {
    const std::map<std::string, std::string>& tags = game.getTags();
    
    // Get fixed column data
    std::string white = tags.contains("White") ? tags.at("White") : "";
    std::string black = tags.contains("Black") ? tags.at("Black") : "";
    
    auto [cause, result] = game.getGameResult();
    std::string resultStr = to_string(result);
    std::string causeStr = to_string(cause);
    // Cause is not set in load games for speed reasons and cannot be used here
    
    std::string moves = std::to_string(game.history().size());
    
    std::vector<std::string> rowData = {white, black, resultStr, causeStr, moves};
    
    for (const std::string& tag : commonTags) {
        // Only add if not already included
        if (!knownTags.contains(tag)) { 
            auto it = tags.find(tag);
            std::string tagValue = (it != tags.end()) ? it->second : "";
            rowData.push_back(tagValue);
        }
    }
    
    return rowData;
}

void ImGuiGameList::createTable() {
    const auto& games = gameRecordManager_.getGames();
    if (games.empty()) {
        return;
    }

    std::scoped_lock lock(gameTableMutex_);

    auto commonTagPairs = gameRecordManager_.getMostCommonTags(10); 
    std::vector<std::string> commonTags;
    commonTags.reserve(commonTagPairs.size());
    for (const auto& pair : commonTagPairs) {
        commonTags.push_back(pair.first);
    }

    // Define fixed columns
    std::vector<ImGuiTable::ColumnDef> columns = {
        { .name = "White", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F },
        { .name = "Black", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F },
        { .name = "Result", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F },
        { .name = "Cause", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F },
        { .name = "PlyCount", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 65.0F, .alignRight = true }
    };

    auto knownTags = std::set<std::string>{};
    for (const auto& col : columns) {
        knownTags.insert(col.name);
    }

    // Add common tag columns
    for (const auto& tag : commonTags) {
        if (!knownTags.contains(tag)) {
            columns.push_back({.name=tag, .flags=ImGuiTableColumnFlags_WidthFixed, .width=100.0F});
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

    // Clear index mapping
    filteredToOriginalIndex_.clear();

    // Fill table with game data (applying filter)
    size_t filteredCount = 0;
    size_t originalIndex = 0;
    for (const auto& game : games) {
        originalIndex++;
        if (!filterPopup_.content().getFilterData().passesFilter(game)) {
            continue;
        }
        filteredCount++;
        
        // Store mapping from filtered index to original index
        filteredToOriginalIndex_.push_back(originalIndex - 1);
        
        auto rowData = createTableRow(game, commonTags, knownTags);
        gameTable_.push(rowData);
    }
    gameTable_.setAutoScroll(true);
    
    // Show filter status in snackbar if filter is active
    const auto& filterData = filterPopup_.content().getFilterData();
    if (filterData.hasActiveFilters()) {
        SnackbarManager::instance().showNote(
            std::format("Filter active: showing {} of {} games", filteredCount, games.size()));
    }

}

void ImGuiGameList::openFile() {
    auto selectedFiles = OsDialogs::openFileDialog(false);
    if (!selectedFiles.empty()) {
        loadFileInBackground(selectedFiles[0]);
    }
}

void ImGuiGameList::loadFileInBackground(const std::string& fileName) {
    // Check if file exists
    if (!std::filesystem::exists(fileName)) {
        SnackbarManager::instance().showWarning("File not found: " + fileName);
        return;
    }
    
    // We disable the filter for new loads
    filterPopup_.content().getFilterData().setActive(false);
    
    // Wait for any previous thread to finish
    if (loadingThread_.joinable()) {
        loadingThread_.join();
    }
    
    // Start loading in background thread
    operationState_.store(OperationState::Loading);
    gamesLoaded_ = 0;
    loadingProgress_ = 0.0F;
    loadingFileName_ = fileName;
    
    loadingThread_ = std::thread(&ImGuiGameList::loadFile, this, fileName);
}

void ImGuiGameList::loadFile(const std::string& fileName) {
    try {
        gamesLoaded_ = 0;
        loadingProgress_ = 0.0F;
        QaplaHelpers::Timer timer;
        timer.start();
        gameRecordManager_.load(fileName, [&](const GameRecord&, float progress) {
            gamesLoaded_++;
            loadingProgress_ = progress;
            return operationState_.load() != OperationState::Cancelling; 
        });
        timer.stop();
        
        bool cancelled = operationState_.load() == OperationState::Cancelling;
        
        const auto& games = gameRecordManager_.getGames();
        gamesLoaded_ = games.size();
        
        // Create table with loaded data
        createTable();
        
        operationState_.store(OperationState::Idle);

        if (cancelled) {
            SnackbarManager::instance().showSuccess(
                std::format("Loading stopped.\n Loaded {} games from {}\nLoading time {} s", games.size(), fileName, 
                    QaplaHelpers::formatMs(timer.elapsedMs())));
        } else {
            SnackbarManager::instance().showSuccess(
                std::format("Loading finished.\n Loaded {} games from {}\nLoading time {} s", games.size(), fileName, 
                    QaplaHelpers::formatMs(timer.elapsedMs())));
        }
    } catch (const std::exception& e) {
        operationState_.store(OperationState::Idle);
        SnackbarManager::instance().showError("Failed to load file: " + std::string(e.what()));
    }
}

void ImGuiGameList::drawGameTable() {
    const auto& games = gameRecordManager_.getGames();
    if (games.empty()) {
        return;
    }

    // Use a child window to ensure proper scrolling
    auto availSize = ImGui::GetContentRegionAvail();
    
    // Only draw if we can acquire the lock without blocking
    if (gameTableMutex_.try_lock()) {
        std::lock_guard lock(gameTableMutex_, std::adopt_lock); // NOLINT(modernize-use-scoped-lock)
        
        auto clickedIndex = gameTable_.draw(ImVec2(0, availSize.y)); 
        if (clickedIndex) {
            gameTable_.setCurrentRow(*clickedIndex);
            // Map filtered index to original game index
            if (*clickedIndex < filteredToOriginalIndex_.size()) {
                size_t originalIndex = filteredToOriginalIndex_[*clickedIndex];
                selectedGame_ = gameRecordManager_.loadGameByIndex(originalIndex);
            } else {
                selectedGame_ = std::nullopt;
            }
        } else {
            selectedGame_ = std::nullopt;
        }
    }
}

void ImGuiGameList::updateFilterConfiguration() {
    filterPopup_.content().updateConfiguration("gamelist");
}

void ImGuiGameList::updateFilterOptions() {
    const auto& games = gameRecordManager_.getGames();
    filterPopup_.content().updateFilterOptions(games);
}

void ImGuiGameList::saveAsFile() {
    const auto& games = gameRecordManager_.getGames();
    if (games.empty()) {
        SnackbarManager::instance().showNote("No games to save");
        return;
    }

    // Open save dialog
    std::vector<std::pair<std::string, std::string>> filters = {
        {"PGN Files", "pgn"},
        {"All Files", "*"}
    };
    std::string selectedFile = OsDialogs::saveFileDialog(filters);
    
    if (selectedFile.empty()) {
        return; // User cancelled
    }

    // Wait for any previous thread to finish
    if (loadingThread_.joinable()) {
        loadingThread_.join();
    }
    
    // Start saving in background thread
    operationState_.store(OperationState::Saving);
    gamesLoaded_ = 0;
    loadingProgress_ = 0.0F;
    savingFileName_ = selectedFile;
    
    loadingThread_ = std::thread(&ImGuiGameList::saveFileInBackground, this, selectedFile);
}

void ImGuiGameList::saveFileInBackground(const std::string& fileName) {
    try {
        // Get filter data reference
        const auto& filterData = filterPopup_.content().getFilterData();
        
        // Create progress callback
        auto progressCallback = [this](size_t gamesSaved, float progress) {
            gamesLoaded_ = gamesSaved;
            loadingProgress_ = progress;
        };
        
        // Create cancel check
        auto cancelCheck = [this]() -> bool {
            return operationState_.load() == OperationState::Cancelling;
        };
        
        // Perform save operation
        size_t gamesSaved = gameRecordManager_.save(fileName, filterData, progressCallback, cancelCheck);
        
        // Update operation state and show success message
        bool cancelled = operationState_.load() == OperationState::Cancelling;
        operationState_.store(OperationState::Idle);
        
        if (cancelled) {
            SnackbarManager::instance().showSuccess(
                std::format("Saving stopped.\nSaved {} games to {}", gamesSaved, fileName));
        } else {
            SnackbarManager::instance().showSuccess(
                std::format("Saving finished.\nSaved {} games to {}", gamesSaved, fileName));
        }
    } catch (const std::exception& e) {
        operationState_.store(OperationState::Idle);
        SnackbarManager::instance().showError("Failed to save file: " + std::string(e.what()));
    }
}
