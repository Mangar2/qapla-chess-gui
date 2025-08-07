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

#include "imgui-table.h"
#include "imgui.h"

namespace QaplaWindows {

    static void textAligned(const std::string& content, bool right) {
        if (right) {
            float region = ImGui::GetContentRegionAvail().x;
            float textWidth = ImGui::CalcTextSize(content.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + region - textWidth);
        }
        ImGui::TextUnformatted(content.c_str());
    }

    ImGuiTable::ImGuiTable(const std::string& tableId,
        ImGuiTableFlags tableFlags,
        const std::vector<ColumnDef>& columns)
        : tableId_(tableId),
        tableFlags_(tableFlags),
        columns_(columns) {
    }

    void ImGuiTable::push(const std::vector<std::string>& row) {
        std::lock_guard<std::mutex> lock(rowsMutex);
        rows_.push_back(row);
    }

    void ImGuiTable::push_front(const std::vector<std::string>& row) {
        std::lock_guard<std::mutex> lock(rowsMutex);
        rows_.insert(rows_.begin(), row);
    }

    void ImGuiTable::clear() {
        std::lock_guard<std::mutex> lock(rowsMutex);
        rows_.clear();
    }

    void ImGuiTable::tableHeadersRow() {
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);

        for (int columnN = 0; columnN < columns_.size(); columnN++) {
            if (!ImGui::TableSetColumnIndex(columnN))
                continue;
			std::string name = columns_[columnN].name;
            ImGui::PushID(columnN);
            ImGui::TableHeader(name.c_str());
            ImGui::PopID();
        }
    }

    void ImGuiTable::draw(const ImVec2& size) {
        if (ImGui::BeginTable(tableId_.c_str(), static_cast<int>(columns_.size()), tableFlags_, size)) {
            for (const auto& column : columns_) {
                ImGui::TableSetupColumn(column.name.c_str(), column.flags, column.width);
            }
            tableHeadersRow();

            std::lock_guard<std::mutex> lock(rowsMutex);
            for (const auto& row : rows_) {
                ImGui::TableNextRow();
                for (size_t col = 0; col < columns_.size() && col < row.size(); ++col) {
                    ImGui::TableSetColumnIndex(static_cast<int>(col));
					textAligned(row[col], columns_[col].alignRight);
                }
            }

            ImGui::EndTable();
        }
    }

}
