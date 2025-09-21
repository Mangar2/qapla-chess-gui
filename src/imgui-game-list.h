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
#include <thread>
#include <atomic>
#include <string>

namespace QaplaWindows {

/**
 * @brief ImGui window for displaying PGN game lists.
 */
class ImGuiGameList : public EmbeddedWindow {
public:
    ImGuiGameList() = default;
    ~ImGuiGameList();

    /**
     * @brief Draws the game list window.
     */
    void draw() override;

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
     * @brief Opens a file dialog and loads the selected PGN file in a background thread.
     */
    void openFile();

    /**
     * @brief Background loading function.
     */
    void loadFileInBackground(const std::string& fileName);

    /**
     * @brief Manager for loaded game records.
     */
    GameRecordManager gameRecordManager_;

    /**
     * @brief Loading state.
     */
    std::atomic<bool> isLoading_{false};

    /**
     * @brief Number of games loaded so far.
     */
    std::atomic<size_t> gamesLoaded_{0};

    /**
     * @brief Loading thread.
     */
    std::thread loadingThread_;

    /**
     * @brief Name of the file being loaded.
     */
    std::string loadingFileName_;
};

} // namespace QaplaWindows
