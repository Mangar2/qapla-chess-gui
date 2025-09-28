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

#include "imgui-table.h"
#include "configuration.h"
#include "snackbar.h"

#include "qapla-tester/engine-worker-factory.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/epd-manager.h"
#include "qapla-tester/game-manager-pool.h"

#include <imgui.h>

namespace QaplaWindows {

    EpdData::EpdData() : 
        Autosavable("epd-result.qepd", ".bak", 60000, []() { return Autosavable::getConfigDirectory(); }),
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
        init();
    }

    EpdData::~EpdData() = default;

    void EpdData:: init() {
        pollCallbackHandle_ = std::move(StaticCallbacks::poll().registerCallback(
		    [this]() {
    			this->pollData();
		    }
    	));
        auto sections = QaplaConfiguration::Configuration::instance().
            getConfigData().getSectionList("epd", "epd").value_or({});
        if (sections.size() > 0) {
            auto section = sections[0];
            epdConfig_ = EpdConfig{
                .filepath = section.getValue("filepath").value_or(""),
                .engines = {},
                .concurrency = QaplaHelpers::to_uint32(section.getValue("concurrency").value_or("")).value_or(1),
                .maxTimeInS = QaplaHelpers::to_uint32(section.getValue("maxtime").value_or("")).value_or(5),
                .minTimeInS = QaplaHelpers::to_uint32(section.getValue("mintime").value_or("")).value_or(1),
                .seenPlies = QaplaHelpers::to_uint32(section.getValue("seenplies").value_or("")).value_or(3)
            };
        }
    }

    void EpdData::updateConfiguration() const {
        QaplaHelpers::IniFile::Section section {
            .name = "epd",
            .entries = QaplaHelpers::IniFile::KeyValueMap{
                {"id", "epd"},
                {"filepath", epdConfig_.filepath},
                {"concurrency", std::to_string(epdConfig_.concurrency)},
                {"maxtime", std::to_string(epdConfig_.maxTimeInS)},
                {"mintime", std::to_string(epdConfig_.minTimeInS)},
                {"seenplies", std::to_string(epdConfig_.seenPlies)}
            }
        };
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("epd", "epd", { section });
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
        totalTests = 0;
        remainingTests = 0;
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
                totalTests++;
                if (test.correct) {
                    table_.extend(row, "d" + std::to_string(test.correctAtDepth) + ", " + QaplaHelpers::formatMs(test.correctAtTimeInMs, 2));
                }
                else if (test.searchDepth >= 0) {
                    table_.extend(row, "-");
                }
                else {
                    remainingTests++;
                    table_.extend(row, "?");
                }
                row++;
            }
            col++;
        }
    }

    void EpdData::pollData() {
        if (updateCnt_ != epdManager_->getUpdateCount()) {
			epdResults_ = std::make_unique<std::vector<EpdTestResult>>(epdManager_->getResultsCopy());
            updateCnt_ = epdManager_->getUpdateCount();
            setModified(); // Notify autosave system about data changes
            if (state == State::Stopping || state == State::Running) {
                if (GameManagerPool::getInstance().runningGameCount() == 0) {
                    state = State::Stopped;
                    SnackbarManager::instance().showSuccess("Analysis finished.");
                }
            }
            populateTable();
		}
    }

    bool EpdData::configChanged() const {
        return scheduledEngines_ == 0 || scheduledConfig_ != epdConfig_;
    }

    void EpdData::analyse() {
        if (configChanged()) {
            if (state == EpdData::State::Stopped) {
                SnackbarManager::instance().showWarning("Configuration changed. Clear data before re-analyzing.");
                return;
            }
            clear();
            epdManager_->initialize(epdConfig_.filepath, epdConfig_.maxTimeInS, epdConfig_.minTimeInS, epdConfig_.seenPlies);
            scheduledConfig_ = epdConfig_;
        }
        state = State::Starting;
        for (uint32_t index = scheduledEngines_; index < epdConfig_.engines.size(); ++index) {
            auto& engineConfig = epdConfig_.engines[index];
            epdManager_->schedule(engineConfig);
            scheduledEngines_++;
        }
        if (epdConfig_.concurrency == 0) {
            epdConfig_.concurrency = std::max<uint32_t>(1, epdConfig_.concurrency);
        }
		GameManagerPool::getInstance().setConcurrency(epdConfig_.concurrency, true, true);
        state = State::Running;
        SnackbarManager::instance().showSuccess("Epd analysis started");
    }

     void EpdData::stopPool(bool graceful) {
        //imguiConcurrency_->setActive(false);
        auto oldState = state;
        state = graceful ? State::Stopping : State::Stopped;
        if (!graceful) {
            GameManagerPool::getInstance().stopAll();
        } else {
            GameManagerPool::getInstance().setConcurrency(0, true, false);
        }

        if (oldState == State::Stopped) {
            SnackbarManager::instance().showNote("No analysis running.");
            return;
        }
        if (oldState == State::Stopping && graceful) {
            SnackbarManager::instance().showNote("Analysis is already stopping gracefully.");
            return;
        }
       
        SnackbarManager::instance().showSuccess(
            graceful ? 
                "Analysis stopped.\nFinishing ongoing calculations." : 
                "Analysis stopped"
        );
    }

    void EpdData::clear() {
        GameManagerPool::getInstance().clearAll();
        epdManager_->clear();
        epdResults_->clear();
        scheduledEngines_ = 0;
        state = State::Cleared;
    }

    void EpdData::setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
         epdConfig_.engines.clear();
        for (const auto& config : configurations) {
            if (config.selected) {
                epdConfig_.engines.push_back(config.config);
            }
        }
        scheduledEngines_ = 0;
    }

    std::optional<size_t> EpdData::drawTable(const ImVec2& size) {
        return table_.draw(size);
    }

    void EpdData::saveData(std::ofstream& out) {
        if (epdManager_) {
            epdManager_->saveResults(out);
        }
    }

    void EpdData::loadData(std::ifstream& in) {
        // TODO: Implement loading of EPD results from file
        // This functionality is not yet implemented in the original code
        (void)in; // Suppress unused parameter warning
    }

}

