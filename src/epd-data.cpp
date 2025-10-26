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

#include <optional>

namespace QaplaWindows {

    using QaplaTester::GameManagerPool;
    using QaplaTester::EpdManager;
    using QaplaTester::EpdTestResult;

    EpdData::EpdData() : 
        Autosavable("epd-result.qepd", ".bak", 60000, []() { return Autosavable::getConfigDirectory(); }),
        epdManager_(std::make_shared<EpdManager>()),
        epdResults_(std::make_unique<std::vector<EpdTestResult>>()),
        engineSelect_(std::make_unique<ImGuiEngineSelect>()),
        table_(
            "EpdResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Name", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 160.0F },
                { .name = "Best move", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 100.0F }
            }
        ),
        viewerBoardWindows_("EPD")
    { 
        setGameManagerPool(std::make_shared<GameManagerPool>());
        table_.setClickable(true);
        setCallbacks();
        init();
    }

    EpdData::~EpdData() = default;

    void EpdData::setCallbacks() {
        pollCallbackHandle_ = std::move(StaticCallbacks::poll().registerCallback(
		    [this]() {
    			this->pollData();
		    }
    	));
        
        engineSelect_->setConfigurationChangedCallback(
            [this](const std::vector<QaplaWindows::ImGuiEngineSelect::EngineConfiguration>& configs) {
                setEngineConfigurations(configs);
            }
        );
    }   

    void EpdData::init() {
        auto sections = QaplaConfiguration::Configuration::instance().
            getConfigData().getSectionList("epd", "epd").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
        if (!sections.empty()) {
            const auto& section = sections[0];
            epdConfig_ = EpdConfig{
                .filepath = section.getValue("filepath").value_or(""),
                .engines = {},
                .maxConcurrency = QaplaHelpers::to_uint32(section.getValue("maxconcurrency").value_or("")).value_or(32),
                .concurrency = QaplaHelpers::to_uint32(section.getValue("concurrency").value_or("")).value_or(1),
                .maxTimeInS = QaplaHelpers::to_uint32(section.getValue("maxtime").value_or("")).value_or(10),
                .minTimeInS = QaplaHelpers::to_uint32(section.getValue("mintime").value_or("")).value_or(1),
                .seenPlies = QaplaHelpers::to_uint32(section.getValue("seenplies").value_or("")).value_or(3)
            };
        } 
        ImGuiEngineSelect::Options options;
        options.allowGauntletEdit = false;
        options.allowPonderEdit = false;
        options.allowTimeControlEdit = false;
        options.allowTraceLevelEdit = true;
        options.allowRestartOptionEdit = false;
        options.allowMultipleSelection = true;
        engineSelect_->setOptions(options);
        auto engineSections = QaplaConfiguration::Configuration::instance().
            getConfigData().getSectionList("engineselection", "epd").
            value_or(std::vector<QaplaHelpers::IniFile::Section>{});
        engineSelect_->setId("epd");
        engineSelect_->setEnginesConfiguration(engineSections);
    }

    void EpdData::updateConfiguration() const {
        QaplaHelpers::IniFile::Section section {
            .name = "epd",
            .entries = QaplaHelpers::IniFile::KeyValueMap{
                {"id", "epd"},
                {"filepath", epdConfig_.filepath},
                {"maxconcurrency", std::to_string(epdConfig_.maxConcurrency)},
                {"concurrency", std::to_string(epdConfig_.concurrency)},
                {"maxtime", std::to_string(epdConfig_.maxTimeInS)},
                {"mintime", std::to_string(epdConfig_.minTimeInS)},
                {"seenplies", std::to_string(epdConfig_.seenPlies)}
            }
        };
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("epd", "epd", { section });
    }

    void EpdData::updateConcurrency(uint32_t newConcurrency) {
        epdConfig_.concurrency = newConcurrency;
        if (state == State::Running) {
            poolAccess_->setConcurrency(epdConfig_.concurrency, true, true);
        }
    }

    std::optional<std::string> EpdData::getFen(size_t index) const {
		if (epdResults_->empty()) {
            return std::nullopt;
        }
		auto& result = (*epdResults_)[0].result;
        if (index < result.size()) {
            auto testCase = result[index];
            return testCase.fen;
        }
        return std::nullopt;
    }

    void EpdData::populateTable() {
        if (!epdResults_) {
            return;
        }
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
                        if (!bestMoves.empty()) {
                            bestMoves += ", ";
                        }
                        bestMoves += move;
                    }
                    row.push_back(bestMoves);
                    table_.push(row);
                }
                table_.setColumnHead(col + 2, {
                    .name = engineName,
                    .flags = ImGuiTableColumnFlags_WidthFixed,
                    .width = 100.0F,
                    .alignRight = true
					});
                totalTests++;
                if (test.correct) {
                    table_.extend(row, "d" + std::to_string(test.correctAtDepth) + ", " + QaplaHelpers::formatMs(test.correctAtTimeInMs, 2));
                }
                else if (!test.playedMove.empty()) {
                    table_.extend(row, "- (" + test.playedMove + ")");
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
                if (poolAccess_->runningGameCount() == 0) {
                    state = State::Stopped;
                    SnackbarManager::instance().showSuccess("Analysis finished.");
                }
            }
            populateTable();
		}
        viewerBoardWindows_.populateViews();
    }

    bool EpdData::configChanged() const {
        return scheduledEngines_ == 0 || scheduledConfig_ != epdConfig_;
    }

    bool EpdData::mayAnalyze(bool sendMessage) const {
        if (epdConfig_.filepath.empty()) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("No EPD or RAW position file selected.");
            }
            return false;
        }
        if (epdConfig_.maxTimeInS == 0) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("Max time must be greater than 0.");
            }
            return false;
        }
        if (epdConfig_.engines.empty()) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("No engines selected for analysis.");
            }
            return false;
        }
        if (totalTests > 0 && remainingTests == 0) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("All tests have been completed. Clear data before re-analyzing.");
            }
            return false;
        }
        if (configChanged() && state == EpdData::State::Stopped) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("Configuration changed. Clear data before re-analyzing.");
            }
            return false;
        }
        if (!poolAccess_->areAllTasksFinished()) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("Some tasks are still running. Please wait until they finish.");
            }
            return false;
        }
        return true;
    }

    void EpdData::analyse() {
        if (!mayAnalyze(true)) {
            return;
        }
        if (configChanged()) {
            clear();
            epdManager_->initialize(epdConfig_.filepath, epdConfig_.maxTimeInS, epdConfig_.minTimeInS, epdConfig_.seenPlies);
            scheduledConfig_ = epdConfig_;
        }
        epdManager_->continueAnalysis();
        state = State::Starting;
		poolAccess_->setConcurrency(epdConfig_.concurrency, true, true);
        for (uint32_t index = scheduledEngines_; index < epdConfig_.engines.size(); ++index) {
            auto& engineConfig = epdConfig_.engines[index];
            epdManager_->schedule(engineConfig, *poolAccess_);
            scheduledEngines_++;
        }
        if (epdConfig_.concurrency == 0) {
            epdConfig_.concurrency = std::max<uint32_t>(1, epdConfig_.concurrency);
        }
        state = State::Running;
        SnackbarManager::instance().showSuccess("Epd analysis started");
    }

     void EpdData::stopPool(bool graceful) {
        //imguiConcurrency_->setActive(false);
        if (state == State::Stopped || state == State::Cleared) {
            SnackbarManager::instance().showNote("No analysis running.");
            return;
        }
        auto oldState = state;
        state = graceful ? State::Stopping : State::Stopped;
        if (!graceful) {
            poolAccess_->stopAll();
        } else {
            poolAccess_->setConcurrency(0, true, false);
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
        poolAccess_->clearAll();
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
        if (epdManager_ && !epdConfig_.filepath.empty()) {
            epdManager_->initialize(epdConfig_.filepath, epdConfig_.maxTimeInS, epdConfig_.minTimeInS, epdConfig_.seenPlies);
            bool dataLoaded = epdManager_->loadResults(in);
            state = dataLoaded ? State::Stopped : State::Cleared;
        }
    }

}

