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
#include <algorithm>

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
    filterData_.init("gamelist");
    filterPopup_.content().setFilterData(&filterData_);
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

    const std::vector<std::string> buttons = {"Open", "Save As", "Filter"};
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
            } else if (button == "Filter") {
                QaplaButton::drawFilter(drawList, topLeft, size, state);
            } else if (button == "Save As") {
                QaplaButton::drawSave(drawList, topLeft, size, state);
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
    } else if (button == "Filter") {
        bool filterActive = filterData_.isActive() && filterData_.hasFilters();
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
            operationState_.store(OperationState::Cancelling);
        } else {
            openFile();
        }
    } else if (!isLoading) {
        if (button == "Save As") {
            SnackbarManager::instance().showNote("Save As button clicked - functionality not yet implemented");
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

    // Fill table with game data (applying filter)
    size_t filteredCount = 0;
    for (const auto& game : games) {
        // Apply filter
        if (!passesFilter(game)) {
            continue;
        }
        filteredCount++;
        
        const std::map<std::string, std::string>& tags = game.getTags();
        
        // Get fixed column data
        std::string white = tags.contains("White") ? tags.at("White") : "";
        std::string black = tags.contains("Black") ? tags.at("Black") : "";
        
        auto [cause, result] = game.getGameResult();
        std::string resultStr = gameResultToPgnResult(result);
        // Cause is not set in load games for speed reasons and cannot be used here
        
        std::string moves = std::to_string(game.history().size());
        
        std::vector<std::string> rowData = {white, black, resultStr, moves};
        
        for (const std::string& tag : commonTags) {
            // Only add if not already included
            if (!knownTags.contains(tag)) { 
                auto it = tags.find(tag);
                std::string tagValue = (it != tags.end()) ? it->second : "";
                rowData.push_back(tagValue);
            }
        }
        
        gameTable_.push(rowData);
    }
    gameTable_.setAutoScroll(true);
    
    // Show filter status in snackbar if filter is active
    if (filterData_.isActive() && filterData_.hasFilters()) {
        SnackbarManager::instance().showNote(
            "Filter active: showing " + std::to_string(filteredCount) + 
            " of " + std::to_string(games.size()) + " games");
    }

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
            selectedGame_ = gameRecordManager_.loadGameByIndex(*clickedIndex);
        } else {
            selectedGame_ = std::nullopt;
        }
    }
}

void ImGuiGameList::updateFilterConfiguration() {
    filterData_.updateConfiguration("gamelist");
}

void ImGuiGameList::updateFilterOptions() {
    const auto& games = gameRecordManager_.getGames();
    if (games.empty()) {
        return;
    }

    // Extract unique player names (both White and Black)
    std::set<std::string> uniquePlayers;
    std::set<QaplaTester::GameResult> uniqueResults;
    std::set<QaplaTester::GameEndCause> uniqueTerminations;

    for (const auto& game : games) {
        const auto& tags = game.getTags();
        
        // Extract both White and Black player names
        auto whiteIt = tags.find("White");
        if (whiteIt != tags.end() && !whiteIt->second.empty()) {
            uniquePlayers.insert(whiteIt->second);
        }
        
        auto blackIt = tags.find("Black");
        if (blackIt != tags.end() && !blackIt->second.empty()) {
            uniquePlayers.insert(blackIt->second);
        }
        
        // Extract game result
        auto [cause, result] = game.getGameResult();
        uniqueResults.insert(result);
        
        // Extract termination cause (only if game ended)
        if (cause != QaplaTester::GameEndCause::Ongoing) {
            uniqueTerminations.insert(cause);
        }
    }

    // Convert sets to vectors for player lists
    std::vector<std::string> playerVec(uniquePlayers.begin(), uniquePlayers.end());
    
    // Sort alphabetically
    std::ranges::sort(playerVec);

    // Update filter window options (same list for both players and opponents)
    filterPopup_.content().setAvailablePlayers(playerVec);
    filterPopup_.content().setAvailableOpponents(playerVec);
    filterPopup_.content().setAvailableResults(uniqueResults);
    filterPopup_.content().setAvailableTerminations(uniqueTerminations);
}

bool ImGuiGameList::passesPlayerNamesFilter(const std::string& white, const std::string& black, 
    const std::set<std::string>& selectedPlayers, const std::set<std::string>& selectedOpponents) {
    if (selectedPlayers.empty() && selectedOpponents.empty()) {
        return true;
    }

    bool playerMatch = selectedPlayers.empty() ||
                      selectedPlayers.contains(white) ||
                      selectedPlayers.contains(black);

    bool opponentMatch = selectedOpponents.empty() ||
                        selectedOpponents.contains(white) ||
                        selectedOpponents.contains(black);

    if (!selectedPlayers.empty() && !selectedOpponents.empty()) {
        bool whitePlayerBlackOpponent = selectedPlayers.contains(white) && selectedOpponents.contains(black);
        bool blackPlayerWhiteOpponent = selectedPlayers.contains(black) && selectedOpponents.contains(white);
        return whitePlayerBlackOpponent || blackPlayerWhiteOpponent;
    }

    return playerMatch && opponentMatch;
}

bool ImGuiGameList::passesResultFilter(QaplaTester::GameResult result, 
    const std::set<QaplaTester::GameResult>& selectedResults) {
    return selectedResults.empty() || selectedResults.contains(result);
}

bool ImGuiGameList::passesTerminationFilter(QaplaTester::GameEndCause cause, 
    const std::set<QaplaTester::GameEndCause>& selectedTerminations) {
    return selectedTerminations.empty() || selectedTerminations.contains(cause);
}

bool ImGuiGameList::passesFilter(const QaplaTester::GameRecord& game) const {
    if (!filterData_.isActive()) {
        return true;
    }

    const auto& tags = game.getTags();
    auto whiteIt = tags.find("White");
    std::string white = (whiteIt != tags.end()) ? whiteIt->second : "";
    auto blackIt = tags.find("Black");
    std::string black = (blackIt != tags.end()) ? blackIt->second : "";

    const auto& selectedPlayers = filterData_.getSelectedPlayers();
    const auto& selectedOpponents = filterData_.getSelectedOpponents();
    if (!passesPlayerNamesFilter(white, black, selectedPlayers, selectedOpponents)) {
        return false;
    }

    const auto& selectedResults = filterData_.getSelectedResults();
    auto [cause, result] = game.getGameResult();
    if (!passesResultFilter(result, selectedResults)) {
        return false;
    }

    const auto& selectedTerminations = filterData_.getSelectedTerminations();
    return passesTerminationFilter(cause, selectedTerminations);
}
