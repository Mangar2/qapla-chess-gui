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

#include "imgui-causes-table.h"
#include "qapla-tester/tournament-result.h"
#include "qapla-tester/game-result.h"
#include <imgui.h>

using namespace QaplaTester;

namespace QaplaWindows {

    ImGuiCausesTable::ImGuiCausesTable()
        : table_(
            "Causes",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Name", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "WDL", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F },
                { .name = "Count", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Cause", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 200.0F }
            }
        ) {
    }

    void ImGuiCausesTable::populate(const std::vector<EngineDuelResult>& duelResults) {
        clear();

        for (const auto& duelResult : duelResults) {
            const std::string& engineName = duelResult.getEngineA();
            
            for (uint32_t index = 0; index < duelResult.causeStats.size(); index++) {
                const auto& stat = duelResult.causeStats[index];
                addRow(engineName, "win", to_string(static_cast<GameEndCause>(index)), stat.win);
            }
            for (uint32_t index = 0; index < duelResult.causeStats.size(); index++) {
                const auto& stat = duelResult.causeStats[index];
                addRow(engineName, "draw", to_string(static_cast<GameEndCause>(index)), stat.draw);
            }
            for (uint32_t index = 0; index < duelResult.causeStats.size(); index++) {
                const auto& stat = duelResult.causeStats[index];
                addRow(engineName, "loss", to_string(static_cast<GameEndCause>(index)), stat.loss);
            }
        }
    }

    std::optional<size_t> ImGuiCausesTable::draw(const ImVec2& size) {
        if (table_.size() == 0) {
            return std::nullopt;
        }
        return table_.draw(size, true);
    }

    void ImGuiCausesTable::clear() {
        table_.clear();
    }

    size_t ImGuiCausesTable::size() const {
        return table_.size();
    }

    void ImGuiCausesTable::addRow(const std::string& name, const std::string& wdl,
                                   const std::string& cause, int count) {
        if (count == 0) {
            return;
        }
        std::vector<std::string> row;
        row.push_back(name);
        row.push_back(wdl);
        row.push_back(std::to_string(count));
        row.push_back(cause);
        table_.push(row);
    }

} // namespace QaplaWindows
