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

#include "analysis-data.h"
#include "configuration.h"
#include "imgui-concurrency.h"
#include "snackbar.h"
#include "ui-thread-watch.h"

#include <analysis/analysis-manager.h>
#include <base-elements/string-helper.h>
#include <game-manager/game-manager-pool.h>

#include <filesystem>
#include <format>

namespace QaplaWindows {

    using QaplaTester::AnalysisDirection;
    using QaplaTester::AnalysisManager;
    using QaplaTester::EngineConfig;
    using QaplaTester::GameManagerPool;
    using QaplaTester::GameRecord;

    namespace {
        constexpr const char* CONFIG_ID = "analysis";
        constexpr const char* SNACKBAR_TOPIC = "analysis";
    }

    AnalysisData::AnalysisData()
        : engineSelect_(std::make_unique<ImGuiEngineSelect>()),
          outputPgn_(std::make_unique<ImGuiTournamentPgn>()),
          imguiConcurrency_(std::make_unique<ImGuiConcurrency>())
    {
        // A pool of its own, like the EPD analysis has: a run here must not take game managers
        // away from a tournament, nor be torn down when one is stopped.
        poolAccess_ = GameManagerPoolAccess(std::make_shared<GameManagerPool>());
        imguiConcurrency_->setPoolAccess(poolAccess_);
        viewerBoardWindows_.setPoolAccess(poolAccess_);
        pollCallbackHandle_ = StaticCallbacks::poll().registerCallback([this]() { this->pollData(); });
        init();
    }

    AnalysisData::~AnalysisData() {
        // The pool waits for its games when it is destroyed, and a run left going would hold up
        // the close for as long as the engines keep searching.
        try {
            static_cast<void>(poolAccess_->stopAll());
        }
        catch (...) { // NOLINT(bugprone-empty-catch)
            // Nothing left to report to on the way out.
        }
    }

    void AnalysisData::init() {
        const auto sections = QaplaConfiguration::Configuration::instance().getConfigData()
            .getSectionList(CONFIG_ID, CONFIG_ID);
        uint32_t concurrency = 1;
        if (sections && !sections->empty()) {
            const auto& section = (*sections)[0];
            config_.inputFile = section.getValue("inputfile").value_or("");
            config_.moveTimeMs = QaplaHelpers::to_uint32(section.getValue("movetime").value_or(""))
                .value_or(1000);
            concurrency = QaplaHelpers::to_uint32(section.getValue("concurrency").value_or(""))
                .value_or(1);
        }

        ImGuiEngineSelect::Options options;
        options.allowGauntletEdit = false;
        options.allowPonderEdit = false;
        // The limit every position is given is the analysis' own setting, one for the whole run.
        options.allowTimeControlEdit = false;
        options.allowTraceLevelEdit = true;
        options.allowRestartOptionEdit = false;
        options.allowMultipleSelection = true;
        engineSelect_->setOptions(options);
        engineSelect_->setId(CONFIG_ID);
        engineSelect_->setEnginesConfiguration(
            QaplaConfiguration::Configuration::instance().getConfigData()
                .getSectionList("engine", CONFIG_ID)
                .value_or(std::vector<QaplaHelpers::IniFile::Section>{}));

        outputPgn_->setId(CONFIG_ID);
        const bool hadOutputConfig = QaplaConfiguration::Configuration::instance().getConfigData()
            .getSectionList("pgnoutput", CONFIG_ID).has_value();
        outputPgn_->loadConfiguration();
        if (!hadOutputConfig) {
            // Every analysed game is worth writing, decided or not: what an analysis produces is
            // the evaluations, and a game that was left unfinished has them just the same. The
            // tournament output defaults the other way round, where an unfinished game means a
            // game that went wrong.
            outputPgn_->pgnOptions().onlyFinishedGames = false;
            outputPgn_->updateConfiguration();
        }

        setExternalConcurrency(concurrency);
    }

    void AnalysisData::updateConfiguration() const {
        QaplaHelpers::IniFile::Section section{ .name = CONFIG_ID, .entries = {} };
        section.addEntry("id", CONFIG_ID);
        section.addEntry("inputfile", config_.inputFile);
        section.addEntry("movetime", std::to_string(config_.moveTimeMs));
        section.addEntry("concurrency", std::to_string(imguiConcurrency_->getExternalConcurrency()));
        QaplaConfiguration::Configuration::instance().getConfigData()
            .setSectionList(CONFIG_ID, CONFIG_ID, { section });
    }

    void AnalysisData::setGames(std::vector<GameRecord> games) {
        games_ = std::move(games);
    }

    uint32_t AnalysisData::getExternalConcurrency() const {
        return imguiConcurrency_->getExternalConcurrency();
    }

    void AnalysisData::setExternalConcurrency(uint32_t count) {
        imguiConcurrency_->setExternalConcurrency(count);
    }

    void AnalysisData::setPoolConcurrency(uint32_t count, bool nice, bool direct) {
        if (!isRunning() && !isStarting()) {
            return;
        }
        imguiConcurrency_->update(count, direct);
        imguiConcurrency_->setNiceStop(nice);
    }

    size_t AnalysisData::getFinishedCount() const {
        size_t finished = 0;
        for (const auto& manager : managers_) {
            finished += manager->getFinishedCount();
        }
        return finished;
    }

    std::vector<EngineConfig> AnalysisData::analysisEngines() const {
        auto engines = engineSelect_->getSelectedEngines();
        for (auto& engine : engines) {
            // Every position is given the same limit; that is what makes the evaluations of one
            // game comparable with each other and with those of the next game.
            engine.setTimeControl(std::format("movetime(ms):{}", config_.moveTimeMs));
        }
        return engines;
    }

    bool AnalysisData::mayAnalyze(bool sendMessage) const {
        auto report = [sendMessage](const std::string& message) {
            if (sendMessage) {
                SnackbarManager::instance().showWarning(message, false, SNACKBAR_TOPIC);
            }
            return false;
        };

        if (isBusy()) {
            return report("An analysis is already running.\nStop it before starting another one.");
        }
        if (games_.empty()) {
            return report("No games to analyse.\nLoad a PGN file first.");
        }
        if (engineSelect_->getSelectedEngines().empty()) {
            return report("No engine selected for the analysis.");
        }
        if (config_.moveTimeMs == 0) {
            return report("Set a time per move greater than zero.");
        }
        if (outputPgn_->pgnOptions().file.empty()) {
            return report("No PGN output file selected.");
        }
        try {
            const std::filesystem::path path(outputPgn_->pgnOptions().file);
            const auto parent = path.parent_path();
            if (!parent.empty() && !std::filesystem::exists(parent)) {
                return report(std::format("The directory of the output file does not exist:\n{}",
                    parent.string()));
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            return report(std::format("The output file cannot be used:\n{}", e.what()));
        }
        if (!poolAccess_->areAllTasksFinished()) {
            return report("The previous analysis has not stopped yet.\nPlease wait a moment.");
        }
        return true;
    }

    void AnalysisData::analyse() {
        if (!mayAnalyze(true)) {
            return;
        }
        try {
            const auto engines = analysisEngines();

            // Truncating happens once for the whole run, not once per engine: every engine
            // appends its own copy of every game, and a second truncation would leave only the
            // last engine's games behind.
            pgnSave_.setOptions(outputPgn_->pgnOptions());
            pgnSave_.initialize("Analysis");

            poolAccess_->clearAll();
            managers_.clear();
            totalCount_ = 0;
            for (const auto& engine : engines) {
                auto manager = std::make_shared<AnalysisManager>();
                manager->setPgnSink(pgnSave_);
                totalCount_ += manager->initialize(games_, AnalysisDirection::Backward);
                manager->startRun(engine);
                managers_.push_back(std::move(manager));
            }
            if (totalCount_ == 0) {
                SnackbarManager::instance().showWarning(
                    "None of the games could be replayed, nothing was analysed.", false, SNACKBAR_TOPIC);
                managers_.clear();
                return;
            }

            state_ = State::Starting;
            sawRunningGame_ = false;
            imguiConcurrency_->init();
            imguiConcurrency_->setActive(true);
            poolAccess_->setConcurrency(getExternalConcurrency(), true, false);
            for (size_t i = 0; i < managers_.size(); ++i) {
                AnalysisManager::schedule(managers_[i], engines[i], *poolAccess_);
            }
            setPoolConcurrency(getExternalConcurrency(), true, true);

            recentInputFiles_.add(config_.inputFile);
            recentOutputFiles_.add(outputPgn_->pgnOptions().file);
            updateConfiguration();

            SnackbarManager::instance().showSuccess(
                std::format("Backward analysis started: {} games, {} engine{}.",
                    games_.size(), engines.size(), engines.size() == 1 ? "" : "s"),
                false, SNACKBAR_TOPIC);
        }
        catch (const std::exception& e) {
            state_ = State::Stopped;
            managers_.clear();
            SnackbarManager::instance().showError(
                std::format("The backward analysis could not be started:\n{}", e.what()),
                false, SNACKBAR_TOPIC);
        }
    }

    void AnalysisData::stop(bool graceful) {
        if (state_ == State::Stopped) {
            SnackbarManager::instance().showNote("No analysis running.", false, SNACKBAR_TOPIC);
            return;
        }
        imguiConcurrency_->stop();
        imguiConcurrency_->setActive(false);

        const auto previous = state_;
        state_ = graceful ? State::Gracefully : State::Stopping;
        if (!graceful) {
            static_cast<void>(poolAccess_->stopAll());
        }
        if (previous == State::Gracefully && graceful) {
            SnackbarManager::instance().showNote(
                "The analysis is already finishing its running games.", false, SNACKBAR_TOPIC);
            return;
        }
        SnackbarManager::instance().showSuccess(
            graceful ? "Analysis stopped.\nThe games already running are being finished."
                     : "Analysis stopped.",
            false, SNACKBAR_TOPIC);
    }

    void AnalysisData::pollData() {
        // Named apart from the other polls so that a frame spent here says which run it was
        // spent on.
        UiThreadWatch::Section section("poll:analysis");

        // Before the state is looked at, and also for a run that has just ended: the boards are a
        // render cache, and a board left half filled from the last frame of a finished run would
        // stay that way until the next run.
        viewerBoardWindows_.populateViews();

        if (state_ == State::Stopped) {
            return;
        }
        if (poolAccess_->runningGameCount() > 0) {
            sawRunningGame_ = true;
            if (state_ == State::Starting) {
                state_ = State::Running;
            }
        }
        // A run can be over before it was ever seen running: a handful of short games on a fast
        // machine are done inside a single frame, and waiting for "a manager is running right
        // now" would leave the analysis calling itself "starting" for ever.
        const bool allDone = totalCount_ > 0 && getFinishedCount() >= totalCount_;
        const bool wasStopped = state_ == State::Stopping || state_ == State::Gracefully;
        if (!allDone && (state_ == State::Starting || poolAccess_->runningGameCount() > 0)) {
            return;
        }

        state_ = State::Stopped;
        if (!sawRunningGame_) {
            // Every game came back without ever having been played: the engine did not start.
            // The pool says so only in the log, which nobody is looking at.
            SnackbarManager::instance().showError(
                "The backward analysis did not run: no game was started.\n"
                "Check that the selected engine starts and answers.",
                false, SNACKBAR_TOPIC);
            return;
        }
        SnackbarManager::instance().showSuccess(
            wasStopped
                ? std::format("Backward analysis stopped: {} of {} games analysed.",
                    getFinishedCount(), totalCount_)
                : "Backward analysis finished.",
            false, SNACKBAR_TOPIC);
    }

}
