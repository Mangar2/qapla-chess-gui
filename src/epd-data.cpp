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

#include "epd-data.h"
#include "qapla-tester/engine-worker-factory.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/epd-manager.h"
#include "qapla-tester/game-manager-pool.h"
#include "imgui-table.h"

#include "imgui.h"

namespace QaplaWindows {

    EpdData::EpdData() : 
        epdManager_(std::make_shared<EpdManager>()),
        epdResults_(std::make_unique<std::vector<EpdTestResult>>()),
        table_(
            "EpdResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { "Name", ImGuiTableColumnFlags_WidthFixed, 160.0f },
                { "Best move", ImGuiTableColumnFlags_WidthFixed, 100.0f },
                { "Result", ImGuiTableColumnFlags_WidthFixed, 100.0f, true }
            }
        )
    { 
        table_.setClickable(true);
    }

    EpdData::~EpdData() = default;

    void EpdData:: init() {
        auto c1 = EngineWorkerFactory::getConfigManager().getConfig("Qapla 0.4.0");
		auto c2 = EngineWorkerFactory::getConfigManager().getConfig("Spike 1.4.1");
        if (!c1 || !c2) return;
        epdConfig_ = EpdConfig{
            .filepath = "test/speelman Endgame.epd",
            .engines = { *c1, *c2 },
            .concurrency = 1,
            .maxTimeInS = 10,
            .minTimeInS = 2,
            .seenPlies = 3
        };
    }

    std::optional<std::string> EpdData::getFen(size_t index) const {
		if (epdResults_->size() == 0) return std::nullopt;
		auto& result = (*epdResults_)[0].result;
        if (index < result.size()) {
            auto testCase = result[index];
            return testCase.fen;
        }
        return std::nullopt;
    }

    void EpdData::populateTable() {
        if (!epdResults_) return;
		table_.clear();
		size_t col = 0; // first two columns are Name and Best Move
        for (auto& result : *epdResults_) {
            size_t row = 0;
            auto& engineName = result.engineName;
            for (auto& test : result.result) {
                if (col == 0) {
                    std::vector<std::string> row{};
                    row.push_back(test.id);
                    std::string bestMoves;
                    for (auto& move : test.bestMoves) {
                        if (!bestMoves.empty()) bestMoves += ", ";
                        bestMoves += move;
                    }
                    row.push_back(bestMoves);
                    table_.push(row);
                }
                table_.setColumnHead(col + 2, {
                    .name = engineName,
                    .flags = ImGuiTableColumnFlags_WidthFixed,
                    .width = 100.0f,
                    .alignRight = true
					});
                if (test.correct) {
                    table_.extend(row, "d" + std::to_string(test.correctAtDepth) + ", " + QaplaHelpers::formatMs(test.correctAtTimeInMs, 2));
                }
                else if (test.searchDepth >= 0) {
                    table_.extend(row, "-");
                }
                else {
                    table_.extend(row, "?");
                }
                row++;
            }
            col++;
        }
    }

    void EpdData::pollData() {
        if (updateCnt != epdManager_->getUpdateCount()) {
			epdResults_ = std::make_unique<std::vector<EpdTestResult>>(epdManager_->getResultsCopy());
            updateCnt = epdManager_->getUpdateCount();
            populateTable();
		}
    }

    void EpdData::analyse() const {
        epdManager_->initialize(
            epdConfig_.filepath, 
            epdConfig_.maxTimeInS, 
            epdConfig_.minTimeInS, 
            epdConfig_.seenPlies);
		GameManagerPool::getInstance().setConcurrency(epdConfig_.concurrency, true);
        for (const auto& engineConfig : epdConfig_.engines) {
            epdManager_->schedule(engineConfig);
		}
    }

    void EpdData::clear() {
        epdManager_->clear();
    }

    std::optional<size_t> EpdData::drawTable(const ImVec2& size) const {
        return table_.draw(size);
    }

}

