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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "book-eval-data.h"
#include "configuration.h"
#include "imgui-concurrency.h"
#include "snackbar.h"

#include <book-eval/book-eval-manager.h>
#include <base-elements/string-helper.h>
#include <game-manager/game-manager-pool.h>

#include <filesystem>
#include <format>

namespace QaplaWindows {

    using QaplaTester::BookEvalConfig;
    using QaplaTester::BookEvalManager;
    using QaplaTester::GameManagerPool;

    namespace {
        constexpr const char* CONFIG_ID = "bookeval";
        constexpr const char* SNACKBAR_TOPIC = "bookeval";
    }

    BookEvalData::BookEvalData()
        : engineSelect_(std::make_unique<ImGuiEngineSelect>()),
          imguiConcurrency_(std::make_unique<ImGuiConcurrency>())
    {
        // A pool of its own, like the analysis has: a run here must not take game managers away
        // from a tournament, nor be torn down when one is stopped.
        poolAccess_ = GameManagerPoolAccess(std::make_shared<GameManagerPool>());
        imguiConcurrency_->setPoolAccess(poolAccess_);
        pollCallbackHandle_ = StaticCallbacks::poll().registerCallback([this]() { this->pollData(); });
        init();
    }

    BookEvalData::~BookEvalData() {
        try {
            static_cast<void>(poolAccess_->stopAll());
            if (manager_) {
                manager_->save();
            }
        }
        catch (...) { // NOLINT(bugprone-empty-catch)
            // Nothing left to report to on the way out.
        }
    }

    void BookEvalData::init() {
        const auto sections = QaplaConfiguration::Configuration::instance().getConfigData()
            .getSectionList(CONFIG_ID, CONFIG_ID);
        uint32_t concurrency = 1;
        if (sections && !sections->empty()) {
            const auto& section = (*sections)[0];
            config_.bookFile = section.getValue("book").value_or("");
            config_.outFile = section.getValue("out").value_or("");
            config_.limit = section.getValue("limit").value_or(LIMIT_DEPTH);
            config_.limitValue = QaplaHelpers::to_uint32(section.getValue("limitvalue").value_or(""))
                .value_or(9);
            config_.saveIntervalS = QaplaHelpers::to_uint32(section.getValue("saveinterval").value_or(""))
                .value_or(60);
            concurrency = QaplaHelpers::to_uint32(section.getValue("concurrency").value_or("")).value_or(1);
        }

        ImGuiEngineSelect::Options options;
        options.allowGauntletEdit = false;
        options.allowPonderEdit = false;
        // The limit of every search is the evaluation's own setting, one for the whole book.
        options.allowTimeControlEdit = false;
        options.allowTraceLevelEdit = true;
        options.allowRestartOptionEdit = false;
        // One engine searches the whole book: scores of two engines in one book mean nothing.
        options.allowMultipleSelection = false;
        engineSelect_->setOptions(options);
        engineSelect_->setId(CONFIG_ID);
        engineSelect_->setEnginesConfiguration(
            QaplaConfiguration::Configuration::instance().getConfigData()
                .getSectionList("engine", CONFIG_ID)
                .value_or(std::vector<QaplaHelpers::IniFile::Section>{}));
        setExternalConcurrency(concurrency);
    }

    void BookEvalData::updateConfiguration() const {
        QaplaHelpers::IniFile::Section section{ .name = CONFIG_ID, .entries = {} };
        section.addEntry("id", CONFIG_ID);
        section.addEntry("book", config_.bookFile);
        section.addEntry("out", config_.outFile);
        section.addEntry("limit", config_.limit);
        section.addEntry("limitvalue", std::to_string(config_.limitValue));
        section.addEntry("saveinterval", std::to_string(config_.saveIntervalS));
        section.addEntry("concurrency", std::to_string(imguiConcurrency_->getExternalConcurrency()));
        QaplaConfiguration::Configuration::instance().getConfigData()
            .setSectionList(CONFIG_ID, CONFIG_ID, { section });
    }

    std::string BookEvalData::suggestedOutFile(const std::string& bookFile) {
        if (bookFile.empty()) {
            return {};
        }
        std::filesystem::path path(bookFile);
        const auto extension = path.has_extension() ? path.extension().string() : std::string(".bok");
        path.replace_filename(path.stem().string() + "-eval" + extension);
        return path.string();
    }

    std::string BookEvalData::timeControl() const {
        if (config_.limit == LIMIT_NODES) {
            return std::format("nodes:{}", config_.limitValue);
        }
        if (config_.limit == LIMIT_MOVETIME) {
            return std::format("movetime(ms):{}", config_.limitValue);
        }
        return std::format("depth:{}", config_.limitValue);
    }

    uint32_t BookEvalData::getExternalConcurrency() const {
        return imguiConcurrency_->getExternalConcurrency();
    }

    void BookEvalData::setExternalConcurrency(uint32_t count) {
        imguiConcurrency_->setExternalConcurrency(count);
    }

    void BookEvalData::setPoolConcurrency(uint32_t count, bool nice, bool direct) {
        if (!isBusy()) {
            return;
        }
        imguiConcurrency_->update(count, direct);
        imguiConcurrency_->setNiceStop(nice);
    }

    size_t BookEvalData::getPositionCount() const {
        return manager_ ? manager_->getPositionCount() : 0;
    }

    size_t BookEvalData::getEvaluatedCount() const {
        return manager_ ? manager_->getEvaluatedCount() : 0;
    }

    double BookEvalData::getPositionsPerSecond() const {
        return manager_ ? manager_->getPositionsPerSecond() : 0.0;
    }

    bool BookEvalData::isComplete() const {
        return manager_ && manager_->isComplete();
    }

    bool BookEvalData::mayEvaluate(bool sendMessage) const {
        auto report = [sendMessage](const std::string& message) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning(message, false, SNACKBAR_TOPIC);
            }
            return false;
        };
        if (isBusy()) {
            return report("A book evaluation is already running.\nStop it before starting another one.");
        }
        if (config_.bookFile.empty() || !std::filesystem::is_regular_file(config_.bookFile)) {
            return report("Choose the book whose positions are evaluated.");
        }
        if (config_.outFile.empty()) {
            return report("Choose the file the evaluated book is written to.");
        }
        if (std::filesystem::absolute(config_.outFile) == std::filesystem::absolute(config_.bookFile)) {
            return report("The evaluated book has to be another file than the book itself.");
        }
        const auto parent = std::filesystem::path(config_.outFile).parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            return report(std::format("The directory of the evaluated book does not exist:\n{}", parent.string()));
        }
        if (engineSelect_->getSelectedEngines().size() != 1) {
            return report("Select one engine to search the positions with.");
        }
        if (config_.limitValue == 0) {
            return report("Set a limit greater than zero.");
        }
        if (!poolAccess_->areAllTasksFinished()) {
            return report("The previous evaluation has not stopped yet.\nPlease wait a moment.");
        }
        return true;
    }

    void BookEvalData::evaluate() {
        if (!mayEvaluate(true)) {
            return;
        }
        try {
            auto engine = engineSelect_->getSelectedEngines().front();
            engine.setTimeControl(timeControl());

            auto manager = std::make_shared<BookEvalManager>();
            const auto open = manager->initialize(BookEvalConfig{
                .bookFile = config_.bookFile,
                .outFile = config_.outFile,
                .saveIntervalMs = config_.saveIntervalS * 1000,
                .maxPositions = 0 }, engine);
            manager_ = manager;
            recentBookFiles_.add(config_.bookFile);
            recentOutFiles_.add(config_.outFile);
            updateConfiguration();
            if (open == 0) {
                SnackbarManager::instance().showSuccess(std::format(
                    "The book is evaluated already: all {} positions have their score.",
                    manager_->getPositionCount()), false, SNACKBAR_TOPIC);
                return;
            }

            poolAccess_->clearAll();
            state_ = State::Starting;
            wasStopped_ = false;
            evaluatedAtStart_ = manager_->getEvaluatedCount();
            imguiConcurrency_->init();
            imguiConcurrency_->setActive(true);
            poolAccess_->setConcurrency(getExternalConcurrency(), true, false);
            BookEvalManager::schedule(manager_, engine, *poolAccess_);
            setPoolConcurrency(getExternalConcurrency(), true, true);

            SnackbarManager::instance().showSuccess(std::format(
                "Book evaluation started: {} of {} positions to search.", open, manager_->getPositionCount()),
                false, SNACKBAR_TOPIC);
        }
        catch (const std::exception& e) {
            state_ = State::Stopped;
            manager_.reset();
            SnackbarManager::instance().showError(
                std::format("The book evaluation could not be started:\n{}", e.what()), false, SNACKBAR_TOPIC);
        }
    }

    void BookEvalData::stop(bool graceful) {
        if (state_ == State::Stopped) {
            SnackbarManager::instance().showNote("No book evaluation running.", false, SNACKBAR_TOPIC);
            return;
        }
        imguiConcurrency_->stop();
        imguiConcurrency_->setActive(false);
        wasStopped_ = true;
        state_ = graceful ? State::Gracefully : State::Stopping;
        if (!graceful) {
            static_cast<void>(poolAccess_->stopAll());
        }
    }

    void BookEvalData::pollData() {
        if (state_ == State::Stopped || !manager_) {
            return;
        }
        if (poolAccess_->runningGameCount() > 0 && state_ == State::Starting) {
            state_ = State::Running;
        }
        // Over once every game manager has run out of positions: after the last search came back,
        // after a stop, and also when the engine never started and every task came to nothing.
        if (manager_->isRunFinished() || poolAccess_->areAllTasksFinished()) {
            finishRun();
        }
    }

    void BookEvalData::finishRun() {
        state_ = State::Stopped;
        imguiConcurrency_->setActive(false);
        try {
            manager_->save();
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showError(
                std::format("The evaluated book could not be written:\n{}", e.what()), false, SNACKBAR_TOPIC);
            return;
        }
        const auto evaluated = manager_->getEvaluatedCount();
        const auto total = manager_->getPositionCount();
        if (evaluated == evaluatedAtStart_ && !wasStopped_) {
            SnackbarManager::instance().showError(
                "The book evaluation did not run: no position was searched.\n"
                "Check that the selected engine starts and answers.", false, SNACKBAR_TOPIC);
            return;
        }
        if (manager_->isComplete()) {
            SnackbarManager::instance().showSuccess(std::format(
                "Book evaluation finished: all {} positions evaluated, written to\n{}", total, config_.outFile),
                false, SNACKBAR_TOPIC);
            return;
        }
        SnackbarManager::instance().showSuccess(std::format(
            "Book evaluation stopped: {} of {} positions evaluated.\nStart it again to continue.",
            evaluated, total), false, SNACKBAR_TOPIC);
    }

}
