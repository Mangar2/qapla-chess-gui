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
#include "pgn-auto-saver.h"
#include "callback-manager.h"
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

    /**
     * @brief The one Pgn view of the application, or nullptr before it exists.
     *
     * There is exactly one, created as a tab at startup and never copied. Anything that wants to
     * put a file in front of the user, or read what is in front of them, goes through here --
     * the backward analysis loads its input file into this view and takes the games it analyses
     * from it, so that what is analysed is what the user can see.
     */
    [[nodiscard]] static ImGuiGameList* instance() { return instance_; }

    /**
     * @brief Loads a PGN file into this view, in the background as the Open button does.
     * @param fileName Path of the file to load.
     */
    void loadPgnFile(const std::string& fileName) { loadFileInBackground(fileName); }

    /** @brief Whether a file is being read right now. */
    [[nodiscard]] bool isLoading() const {
        const auto state = operationState_.load();
        return state == OperationState::Loading || state == OperationState::Cancelling;
    }

    /** @brief The file the loaded games came from. */
    [[nodiscard]] const std::string& getLoadedFileName() const {
        return gameRecordManager_.getCurrentFileName();
    }

    /** @brief The number of games loaded, before filtering. */
    [[nodiscard]] size_t getLoadedGameCount() const {
        return isLoading() ? 0 : gameRecordManager_.getGames().size();
    }

    /** @brief The number of games the filter lets through. */
    [[nodiscard]] size_t getFilteredGameCount() const;

    /**
     * @brief The games the filter lets through, as copies.
     *
     * Copies rather than a view: the caller keeps them and works on them while this view goes on
     * to hold something else entirely. Empty while a file is being read.
     */
    [[nodiscard]] std::vector<QaplaTester::GameRecord> getFilteredGames() const;

    /**
     * @brief Asks for the filter dialog of this view, the same one its Filter button opens.
     *
     * Opened on the next frame by this view itself, not here: an ImGui popup belongs to the
     * window that opens it, and one opened from the chat panel would be a popup of the chat
     * panel -- drawn nowhere, since it is this view that draws it.
     */
    void requestFilterDialog() { filterRequested_ = true; }

    /** @brief Whether the filter dialog is open or about to be. */
    [[nodiscard]] bool isFilterDialogOpen() const { return filterRequested_ || filterPopup_.isOpen(); }

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
     * @brief Opens the filter dialog, from this view's own drawing.
     */
    void openFilterDialog();

    /**
     * @brief Starts loading a file in background thread with validation.
     */
    void loadFileInBackground(const std::string& fileName);

    /**
     * @brief Loads a file (runs in background thread).
     */
    void loadFile(const std::string& fileName);

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
    mutable std::mutex gameTableMutex_;

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

    /** @brief The one instance; see instance(). */
    inline static ImGuiGameList* instance_ = nullptr;

    /** @brief Set by requestFilterDialog(), acted on in draw(). */
    bool filterRequested_ = false;

    /**
     * @brief How many games the filter lets through, as of the last time the table was built.
     *
     * Kept beside filteredToOriginalIndex_ rather than read from it: the table is built on the
     * loading thread, and a caller drawing a frame must be able to ask this without waiting for
     * that thread and without reading a vector while it grows.
     */
    std::atomic<size_t> filteredCount_{0};

    std::pair<QaplaButton::ButtonState, std::string> computeButtonState(const std::string& button, bool isLoading) const;
    void executeCommand(const std::string& button, bool isLoading);

    /**
     * @brief Subscription to StaticCallbacks::message() for "load_pgn_file:<path>" -- lets the
     * AI-chatbot's open_pgn_file tool (see src/llm/actions/gui-action-app.cpp) load a specific file into
     * this tab without needing a singleton/pointer to this (non-singleton) instance.
     */
    std::unique_ptr<QaplaWindows::Callback::UnregisterHandle> messageCallbackHandle_;
};

} // namespace QaplaWindows
