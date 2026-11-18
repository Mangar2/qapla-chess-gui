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

#include "ui-thread-watch.h"
#include "epd-data.h"

#include "imgui-table.h"
#include "configuration.h"
#include "snackbar.h"
#include "imgui-concurrency.h"

#include <engine-handling/engine-worker-factory.h>
#include <base-elements/string-helper.h>
#include "epd/epd-manager.h"
#include <game-manager/game-manager-pool.h>

#include "ui/qapla-ui.h"

#include <optional>
#include <thread>
#include <chrono>
#include <string>

namespace QaplaWindows {

    using QaplaTester::GameManagerPool;
    using QaplaTester::EpdManager;
    using QaplaTester::EpdTestResult;

    EpdData::EpdData() : 
        Autosavable("epd-result.qepd", ".bak", 60000, []() { return Autosavable::getConfigDirectory(); }),
        epdManager_(std::make_shared<EpdManager>()),
        epdResults_(std::make_unique<std::vector<EpdTestResult>>()),
        engineSelect_(std::make_unique<ImGuiEngineSelect>()),
        imguiConcurrency_(std::make_unique<ImGuiConcurrency>()),
        table_(
            "EpdResult",
            QaplaUi::TableFlags::RowBg | QaplaUi::TableFlags::SizingFixedFit | QaplaUi::TableFlags::ScrollX | QaplaUi::TableFlags::ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Name", .flags = QaplaUi::TableColumnFlags::WidthFixed, .width = 160.0F },
                { .name = "Best move", .flags = QaplaUi::TableColumnFlags::WidthFixed, .width = 100.0F }
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
        pollCallbackHandle_ = StaticCallbacks::poll().registerCallback(
		    [this]() {
    			this->pollData();
		    }
    	);
        saveCallbackHandle_ = StaticCallbacks::save().registerCallback(
            [this]() {
                this->saveFile();
            }
        );
        
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
            getConfigData().getSectionList("engine", "epd").
            value_or(std::vector<QaplaHelpers::IniFile::Section>{});
        engineSelect_->setId("epd");
        engineSelect_->setEnginesConfiguration(engineSections);
        
        // Initialize external concurrency from saved config
        setExternalConcurrency(epdConfig_.concurrency);
    }

    void EpdData::updateConfiguration() const {
        QaplaHelpers::IniFile::Section section {
            .name = "epd",
            .entries = QaplaHelpers::IniFile::KeyValueMap{
                {"id", "epd"},
                {"filepath", epdConfig_.filepath},
                {"maxconcurrency", std::to_string(epdConfig_.maxConcurrency)},
                {"concurrency", std::to_string(imguiConcurrency_->getExternalConcurrency())},
                {"maxtime", std::to_string(epdConfig_.maxTimeInS)},
                {"mintime", std::to_string(epdConfig_.minTimeInS)},
                {"seenplies", std::to_string(epdConfig_.seenPlies)}
            }
        };
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList("epd", "epd", { section });
    }

    uint32_t EpdData::getExternalConcurrency() const {
        return imguiConcurrency_->getExternalConcurrency();
    }

    void EpdData::setExternalConcurrency(uint32_t count) {
        imguiConcurrency_->setExternalConcurrency(count);
    }

    void EpdData::stopPoolAbruptlyAndWait() {
        stopPool(false);
        // Drains the pool instead of waiting for a later frame to notice -- that never happens
        // while this call is holding the UI thread anyway.
        {
            // The user interface may block here, and that is not a fault of its own: it is
            // waiting for engines to stop. Taken out of the frame -- see UiThreadWatch::Waiting.
            UiThreadWatch::Waiting waiting(std::string(UiThreadWatch::POOL_SECTION) + ":epd-wait");
            poolAccess_->waitForTask();
        }
        state = State::Stopped;
    }

    void EpdData::setPoolConcurrency(uint32_t count, bool nice, bool direct) {
        if (!isRunning() && !isStarting()) {
            return;
        }
        imguiConcurrency_->update(count, direct);
        imguiConcurrency_->setNiceStop(nice);
    }

    void EpdData::setGameManagerPool(const std::shared_ptr<QaplaTester::GameManagerPool>& pool) {
        poolAccess_ = GameManagerPoolAccess(pool);
        viewerBoardWindows_.setPoolAccess(poolAccess_);
        imguiConcurrency_->setPoolAccess(poolAccess_);
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

    namespace {
        /** @brief What the table shows for one engine's result at a position. */
        std::string resultCell(const QaplaTester::EpdTestCase& test) {
            if (test.correct) {
                return "d" + std::to_string(test.correctAtDepth) + ", "
                    + QaplaHelpers::formatMs(test.correctAtTimeInMs, 2);
            }
            if (!test.playedMove.empty()) {
                return "- (" + test.playedMove + ")";
            }
            return "?";
        }

        /** @brief Whether the position still waits for this engine's result. */
        bool isRemaining(const QaplaTester::EpdTestCase& test) {
            return !test.correct && test.playedMove.empty();
        }

        constexpr size_t FIRST_ENGINE_COLUMN = 2; ///< Name and best move come first
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
            size_t rowIndex = 0;
            auto& engineName = result.engineName;            
            for (auto& test : result.result) {
                if (col == 0) {
                    std::vector<std::string> row{};
                    std::string id = test.id.empty() ? std::to_string(rowIndex) : test.id;
                    row.push_back(id);
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
                table_.setColumnHead(col + FIRST_ENGINE_COLUMN, {
                    .name = engineName,
                    .flags = QaplaUi::TableColumnFlags::WidthFixed,
                    .width = 100.0F,
                    .alignRight = true
					});
                totalTests++;
                if (isRemaining(test)) {
                    remainingTests++;
                }
                table_.extend(rowIndex, resultCell(test));
                rowIndex++;
            }
            col++;
        }
    }

    void EpdData::refreshResults() {
        auto changes = epdManager_->getChangesSince(seenCounts_);
        // Nothing taken yet -- the results were cleared or replaced -- is a rebuild as well, even
        // with no engine left: the table and the counts still show what was there before.
        bool rebuild = seenCounts_.empty() || changes.size() != epdResults_->size();
        epdResults_->resize(changes.size());
        seenCounts_.resize(changes.size());
        for (size_t engine = 0; engine < changes.size(); ++engine) {
            auto& change = changes[engine];
            seenCounts_[engine] = change.updateCount;
            auto& results = (*epdResults_)[engine].result;
            if (change.complete) {
                (*epdResults_)[engine] = std::move(change.all);
                rebuild = true;
                continue;
            }
            for (auto& [index, test] : change.changed) {
                if (index >= results.size()) {
                    rebuild = true;
                    continue;
                }
                auto& stored = results[index];
                if (isRemaining(stored) && !isRemaining(test) && remainingTests > 0) {
                    remainingTests--;
                } else if (!isRemaining(stored) && isRemaining(test)) {
                    remainingTests++;
                }
                stored = std::move(test);
                if (!rebuild) {
                    table_.setField(index, FIRST_ENGINE_COLUMN + engine, resultCell(stored));
                }
            }
        }
        if (rebuild) {
            populateTable();
        }
    }

    void EpdData::pollData() {
        // Named apart from the other polls so that a frame spent here says which run
        // it was spent on -- the same reason a tool call is timed as "tool:<name>".
        QaplaWindows::UiThreadWatch::Section section("poll:epd");

        if (updateCnt_ != epdManager_->getUpdateCount()) {
            updateCnt_ = epdManager_->getUpdateCount();
            setModified(); // Notify autosave system about data changes
            refreshResults();
		}
        if (state == State::Starting && poolAccess_->runningGameCount() > 0) {
            state = State::Running;
        }
        // A run can be over before it was ever seen running. Leaving Starting only by way of
        // "a manager is running right now" is a guess about a moment, and four positions on a
        // fast machine are done inside a single frame: no poll ever sees one. Measured on
        // Windows, where the results were complete after a tenth of a second and the analysis
        // still called itself "starting" five minutes later, so nothing that waits for it to
        // finish ever came back. Every position carrying a result is not a guess.
        if (state == State::Starting && allPositionsTested()) {
            state = State::Stopped;
            SnackbarManager::instance().showSuccess("Analysis finished.", false, "epd");
        }
        if (state == State::Stopping || state == State::Gracefully || state == State::Running) {
            if (poolAccess_->runningGameCount() == 0) {
                state = State::Stopped;
                SnackbarManager::instance().showSuccess("Analysis finished.", false, "epd");
            }
        }

        viewerBoardWindows_.populateViews();
    }

    bool EpdData::allPositionsTested() const {
        if (!epdResults_ || epdResults_->empty()) {
            return false;
        }
        for (const auto& engineResult : *epdResults_) {
            if (engineResult.result.empty()) {
                return false;
            }
            for (const auto& testCase : engineResult.result) {
                if (!testCase.tested) {
                    return false;
                }
            }
        }
        return true;
    }

    bool EpdData::configChanged() const {
        return scheduledEngines_ == 0 || scheduledConfig_ != epdConfig_;
    }

    bool EpdData::mayAnalyze(bool sendMessage) const {
        if (epdConfig_.filepath.empty()) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("No EPD or RAW position file selected.",
                    false, "epd");
            }
            return false;
        }
        if (epdConfig_.maxTimeInS == 0) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("Max time must be greater than 0.",
                    false, "epd");
            }
            return false;
        }
        if (epdConfig_.engines.empty()) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("No engines selected for analysis.",
                    false, "epd");
            }
            return false;
        }
        if (totalTests > 0 && remainingTests == 0) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("All tests have been completed. Clear data before re-analyzing.",
                    false, "epd");
            }
            return false;
        }
        if (configChanged() && state == EpdData::State::Stopped) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("Configuration changed. Clear data before re-analyzing.",
                    false, "epd");
            }
            return false;
        }
        if (state == State::Stopping || state == State::Gracefully) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("Analysis is stopping. Please wait until it has fully stopped.",
                    false, "epd");
            }
            return false;
        }
        if (!poolAccess_->areAllTasksFinished()) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning("Some tasks are still running. Please wait until they finish.",
                    false, "epd");
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
            initializeManager();
            scheduledConfig_ = epdConfig_;
        }

        epdManager_->continueAnalysis();
        state = State::Starting;
        for (uint32_t index = scheduledEngines_; index < epdConfig_.engines.size(); ++index) {
            auto& engineConfig = epdConfig_.engines[index];
            epdManager_->schedule(engineConfig, *poolAccess_);
            scheduledEngines_++;
        }
        
        imguiConcurrency_->init();
        imguiConcurrency_->setActive(true);

        // getExternalConcurrency() is always >= 1 (see ImGuiConcurrency's class doc comment),
        // so no defensive floor is needed here anymore.
        setPoolConcurrency(getExternalConcurrency(), true, true);

        SnackbarManager::instance().showSuccess("Epd analysis started", false, "epd");
    }

     void EpdData::stopPool(bool graceful) {
        // Must be called before deactivating the control so that the pool is informed about
        // setting concurrency to zero. Uses stop() (not update(0)) so the user's configured
        // concurrency survives the stop -- see ImGuiConcurrency's class doc comment.
        imguiConcurrency_->stop();
        // Prevents that concurrency control tells the pool to start new tasks when calculations are stopped
        imguiConcurrency_->setActive(false);

        if (state == State::Stopped || state == State::Cleared) {
            SnackbarManager::instance().showNote("No analysis running.", false, "epd");
            return;
        }
        auto oldState = state;
        state = graceful ? State::Gracefully : State::Stopping;
        if (!graceful) {
            // If we are not graceful, we stop all immediately
            poolAccess_->stopAll();
        } 

        if (oldState == State::Gracefully && graceful) {
            SnackbarManager::instance().showNote("Analysis is already stopping gracefully.", 
                false, "epd");
            return;
        }
       
        SnackbarManager::instance().showSuccess(
            graceful ? 
                "Analysis stopped.\nFinishing ongoing calculations." : 
                "Analysis stopped",
            false, "epd"
        );
    }

    void EpdData::clear() {
        {
            // The user interface may block here, and that is not a fault of its own: it is
            // waiting for engines to stop. Taken out of the frame -- see UiThreadWatch::Waiting.
            UiThreadWatch::Waiting waiting(std::string(UiThreadWatch::POOL_SECTION) + ":epd-clear");
            poolAccess_->clearAll();
        }
        epdManager_->clear();
        epdResults_->clear();
        forgetSeenResults();
        scheduledEngines_ = 0;
        state = State::Cleared;
    }

    void EpdData::setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
         epdConfig_.engines.clear();
        for (const auto& config : configurations) {
            if (config.isSelected()) {
                epdConfig_.engines.push_back(config);
            }
        }
        scheduledEngines_ = 0;
    }

    std::optional<size_t> EpdData::drawTable(const QaplaUi::Vec2& size) {
        return table_.draw(size);
    }

    std::string EpdData::resultsAsText() {
        // Brought up to date first -- see SprtTournamentData::resultsAsText() for why reading the
        // render cache alone answered "no results" for a finished run still on screen. Not
        // refilled from scratch: with 600 000 positions that cost a rebuild of every row for
        // every question, and refreshResults() rebuilds whenever the results were replaced.
        refreshResults();
        return table_.toText();
    }

    TableContents EpdData::resultsAsTable() {
        // Brought up to date first -- see resultsAsText() for why.
        refreshResults();
        return table_.contents();
    }

    std::string EpdData::saveResultsTo(const std::string& filename) {
        if (!epdManager_) {
            return "the EPD analysis has not been set up yet";
        }
        try {
            std::ofstream out(filename, std::ios::trunc);
            if (!out) {
                return "the file could not be opened for writing";
            }
            epdManager_->saveResults(out);
            if (!out.good()) {
                return "writing stopped part-way through";
            }
        }
        catch (const std::exception& e) {
            return e.what();
        }
        return {};
    }

    std::string EpdData::loadResultsFrom(const std::string& filename) {
        if (!epdManager_) {
            return "the EPD analysis has not been set up yet";
        }
        if (epdConfig_.filepath.empty()) {
            // The stored results name positions but do not carry them, so without a position file
            // there is nothing to match them onto -- and initialize() below would have nothing to
            // read either. Refusing here says why; going ahead would just load nothing.
            return "no EPD position file is configured to match the stored results against";
        }
        try {
            std::ifstream in(filename);
            if (!in) {
                return "the file could not be opened for reading";
            }
            initializeManager();
            if (!epdManager_->loadResults(in)) {
                return "the file holds no results for the configured position set";
            }
            state = State::Stopped;
        }
        catch (const std::exception& e) {
            return e.what();
        }

        // pollData() would pick this up on the next frame anyway, but a caller that asks for its
        // results in the very same turn must not be told the old figures -- see resultsAsText().
        updateCnt_ = epdManager_->getUpdateCount();
        refreshResults();
        setModified();
        return {};
    }

    void EpdData::initializeManager() {
        epdManager_->initialize(epdConfig_.filepath, epdConfig_.maxTimeInS, epdConfig_.minTimeInS,
            epdConfig_.seenPlies, 0, 0, MAX_EPD_POSITIONS);
        forgetSeenResults();
        if (auto note = cutOffNote(); !note.empty()) {
            SnackbarManager::instance().showWarning(note, false, "epd");
        }
    }

    std::string EpdData::cutOffNote() const {
        if (!epdManager_ || !epdManager_->wasTruncated()) {
            return {};
        }
        const auto limit = std::to_string(MAX_EPD_POSITIONS);
        return "The EPD file holds more than " + limit + " positions. Only the first " + limit
            + " were read, the rest is cut off.";
    }

    void EpdData::saveData(std::ofstream& out) {
        if (epdManager_) {
            epdManager_->saveResults(out);
        }
    }

    void EpdData::loadData(std::ifstream& in) {
        if (epdManager_ && !epdConfig_.filepath.empty()) {
            initializeManager();
            bool dataLoaded = epdManager_->loadResults(in);
            state = dataLoaded ? State::Stopped : State::Cleared;
        }
    }

}

