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

#include "imgui-table.h"
#include <vector>
#include <string>
#include <memory>

// Forward declarations
namespace QaplaTester {
    struct EngineDuelResult;
}

namespace QaplaWindows {

    /**
     * @brief ImGui component for displaying game termination causes in a table.
     * @details This component provides a fixed-configuration table showing the causes
     *          for game termination (wins, draws, losses) categorized by engine and cause.
     */
    class ImGuiCausesTable {
    public:
        ImGuiCausesTable();
        ~ImGuiCausesTable() = default;

        /**
         * @brief Populates the causes table with engine duel results.
         * @param duelResults Vector of engine duel results to display
         */
        void populate(const std::vector<QaplaTester::EngineDuelResult>& duelResults);

        /**
         * @brief Draws the causes table.
         * @param size Size of the table to draw.
         * @return The index of the selected row, or std::nullopt if no row was selected.
         */
        std::optional<size_t> draw(const ImVec2& size);

        /**
         * @brief Clears the table data.
         */
        void clear();

        /**
         * @brief Returns the number of rows in the table.
         * @return Number of rows.
         */
        size_t size() const;

    private:
        ImGuiTable table_;

        /**
         * @brief Adds a row to the causes table if count > 0.
         * @param name Engine name
         * @param wdl Win/Draw/Loss indicator
         * @param cause The cause of game termination
         * @param count Number of games with this cause
         */
        void addRow(const std::string& name, const std::string& wdl, 
                   const std::string& cause, int count);
    };

} // namespace QaplaWindows
