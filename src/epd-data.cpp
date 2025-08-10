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
#include "imgui-table.h"

#include "imgui.h"

namespace QaplaWindows {

    EpdData::EpdData() : 
        epdManager_(std::make_shared<EpdManager>()),
        epdTests_(std::make_unique<std::vector<EpdTestCase>>()),
        epdResults_(std::make_unique<std::vector<EpdTestResult>>()),
        table_(
            "EpdResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { "Name", ImGuiTableColumnFlags_WidthFixed, 200.0f },
                { "Best move", ImGuiTableColumnFlags_WidthFixed, 80.0f },
                { "Result", ImGuiTableColumnFlags_WidthFixed, 100.0f, true }
            }
        )
    { 
        auto config = EngineWorkerFactory::getConfigManager().getConfig("Qapla 0.4.0");
        if (!config) return;
        epdConfig_ = EpdConfig{
            .filepath = "test/speelman Endgame.epd",
            .engine = *config,
            .concurrency = 1,
            .maxTimeInS = 10,
            .minTimeInS = 2,
            .seenPlies = 3
        };
    }

    EpdData::~EpdData() = default;

    void EpdData::populateTable() {
        if (!epdTests_) return;
		table_.clear();
        for (auto& test : *epdTests_) {
            std::vector<std::string> row{};
            row.push_back(test.id);
            std::string bestMoves;
            for (auto& move : test.bestMoves) {
                if (!bestMoves.empty()) bestMoves += ", ";
                bestMoves += move;
            }
            row.push_back(bestMoves);
            if (test.correct) {
                row.push_back("d" + std::to_string(test.correctAtDepth) + ", " + formatMs(test.correctAtTimeInMs, 2));
            } 
            else if (test.searchDepth >= 0) {
                row.push_back("-");
            }
            else {
                row.push_back("?");
            }
			table_.push(row);
        }

    }

    void EpdData::pollData() {
        if (updateCnt != epdManager_->getUpdateCount()) {
            epdTests_ = std::make_unique<std::vector<EpdTestCase>>(epdManager_->getTestsCopy());
			epdResults_ = std::make_unique<std::vector<EpdTestResult>>(epdManager_->getResultsCopy());
            updateCnt = epdManager_->getUpdateCount();
            populateTable();
		}
    }

    void EpdData::analyse() const {
        epdManager_->analyzeEpd(
            epdConfig_.filepath, 
            epdConfig_.engine, 
            epdConfig_.concurrency, 
            epdConfig_.maxTimeInS, 
            epdConfig_.minTimeInS, 
            epdConfig_.seenPlies);
        epdManager_->schedule(epdManager_, epdConfig_.engine);
    }

    void EpdData::drawTable(const ImVec2& size) const {
        table_.draw(size);
    }

}

