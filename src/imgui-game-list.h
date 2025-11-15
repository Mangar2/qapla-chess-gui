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
#include "game-record-manager.h"
#include "game-filter-window.h"
#include "imgui-table.h"
#include "imgui-popup.h"
#include "imgui-button.h"
#include <thread>
#include <atomic>
#include <string>
#include <mutex>

namespace QaplaWindows {

/**
 * @brief State for background operations.
 */
enum class OperationState {
    Idle,       ///< No operation in progress
    Loading,    ///< Currently loading
    Cancelling, ///< Operation is being cancelled
    Saving,     ///< Currently saving (future use)
    Filtering   ///< Currently filtering (future use)
};

/**
 * @brief ImGui window for displaying PGN game lists.
 */
class ImGuiGameList : public EmbeddedWindow {
public:
    ImGuiGameList();
    ~ImGuiGameList();

    /**
     * @brief Initializes the game list (loads filter configuration).
     */
    void init();

    /**
     * @brief Draws the game list window.
     */
    void draw() override;

    /**
     * @brief Get the Selected Game object
     * 
     * @return std::optional<GameRecord> 
     */
    static std::optional<QaplaTester::GameRecord> getSelectedGame() {
        return selectedGame_;
    }

private:
    /**
     * @brief Draws the buttons for the game list.
     */
    void drawButtons();

    /**
     * @brief Draws the loading status below the buttons.
     */
    void drawLoadingStatus();

    /**
     * @brief Creates and fills the game table with loaded data.
     */
    void createTable();

    /**
     * @brief Draws the game table if games are loaded.
     */
    void drawGameTable();

    /**
     * @brief Opens a file dialog and loads the selected PGN file in a background thread.
     */
    void openFile();

    /**
     * @brief Saves the current games to a new file.
     */
    void saveAsFile();

    /**
     * @brief Updates the filter configuration.
     */
    void updateFilterConfiguration();

    /**
     * @brief Extracts unique values from loaded games for filter options.
     */
    void updateFilterOptions();

    /**
     * @brief Background loading function.
     */
    void loadFileInBackground(const std::string& fileName);

    /**
     * @brief Background saving function.
     */
    void saveFileInBackground(const std::string& fileName);

    /**
     * @brief Manager for loaded game records.
     */
    GameRecordManager gameRecordManager_;

    /**
     * @brief Current operation state.
     */
    std::atomic<OperationState> operationState_{OperationState::Idle};

    /**
     * @brief Number of games loaded so far.
     */
    std::atomic<size_t> gamesLoaded_{0};

    /**
     * @brief Loading progress percentage (0-100).
     */
    std::atomic<float> loadingProgress_{0.0F};

    /**
     * @brief Loading thread.
     */
    std::thread loadingThread_;

    /**
     * @brief Name of the file being loaded.
     */
    std::string loadingFileName_;

    /**
     * @brief Name of the file being saved.
     */
    std::string savingFileName_;

    /**
     * @brief Table for displaying game data.
     */
    ImGuiTable gameTable_;

    /**
     * @brief Mutex for synchronizing access to the game table.
     */
    std::mutex gameTableMutex_;

    /**
     * @brief Popup window for filter configuration.
     */
    ImGuiPopup<GameFilterWindow> filterPopup_;

    /**
     * @brief Maps filtered table row index to original game index.
     * Index in this vector is the row in the table, value is the original game index.
     */
    std::vector<size_t> filteredToOriginalIndex_;

    inline static std::optional<QaplaTester::GameRecord> selectedGame_;

    std::pair<QaplaButton::ButtonState, std::string> computeButtonState(const std::string& button, bool isLoading) const;
    void executeCommand(const std::string& button, bool isLoading);
};

} // namespace QaplaWindows
