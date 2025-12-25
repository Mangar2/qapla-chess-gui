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


#include "tournament-data.h"
#include "tournament-result-incremental.h"
#include "tournament-result-view.h"
#include "viewer-board-window.h"
#include "viewer-board-window-list.h"
#include "snackbar.h"
#include "imgui-concurrency.h"
#include "imgui-engine-global-settings.h"
#include "configuration.h"

#include "string-helper.h"

#include "engine-option.h"
#include "engine-worker-factory.h"

#include "tournament.h"
#include "tournament-result.h"

#include "game-manager-pool.h"
#include "pgn-io.h"
#include "pgn-save.h"
#include "adjudication-manager.h"
#include "imgui-table.h"

#include <format>



using namespace QaplaTester;

namespace QaplaWindows {

    TournamentData::TournamentData() : 
        eloTable_(
            "TournamentResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Name", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Elo", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Error", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Score", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 100.0F, .alignRight = true },
                { .name = "%", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Total", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true }
            }
        ),
        runningTable_(
            "Running",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "White", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Black", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Round", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F },
                { .name = "Game", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F },
                { .name = "Opening", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F }
            }
		),
        adjudicationTable_(
            "Adjudication Tests",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Adjudicate", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F },
                { .name = "Total", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Correct", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 60.0F, .alignRight = true },
                { .name = "Incorrect", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 70.0F, .alignRight = true },
                { .name = "Saveable", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F, .alignRight = true },
                { .name = "Total Time", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F, .alignRight = true }
            }
        ),
        matrixTable_(
            "Results Matrix",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Rank", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Engine", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Score", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 100.0F, .alignRight = true },
                { .name = "%", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true }
            }
        )
    { 
        setGameManagerPool(std::make_shared<GameManagerPool>());
        runningTable_.setClickable(true);
        
        // Set up engine select options
        ImGuiEngineSelect::Options options;
        options.allowGauntletEdit = true;
        options.allowPonderEdit = true;
        options.allowTimeControlEdit = true;
        options.allowTraceLevelEdit = true;
        options.allowRestartOptionEdit = true;
        options.allowMultipleSelection = true;
        engineSelect_->setOptions(options);
        
        tournamentOpening_->setId("tournament");
        tournamentPgn_->setId("tournament");
        tournamentAdjudication_->setId("tournament");
        tournamentConfiguration_->setId("tournament");
        tournamentConfiguration_->setConfig(config_.get());
        matrixTable_.setFont(FontManager::ibmPlexMonoIndex);
        eloTable_.setFont(FontManager::ibmPlexMonoIndex);

        // Set up callbacks
        setupCallbacks();
        
        loadConfig();
        loadTournament();
    }

    TournamentData::~TournamentData() = default;

    void TournamentData::setupCallbacks() {
        // Callback is triggered whenever the user modifies global engine settings in the UI
        // It copies the settings from the UI component into eachEngineConfig_, which is later
        // used in createTournament() to apply these global settings to all selected engines
        globalSettings_->setConfigurationChangedCallback(
            [this](const ImGuiEngineGlobalSettings::GlobalConfiguration& settings) {
                eachEngineConfig_ = settings;
            }
        );
        
        // Callback is triggered when the user changes time control settings in the UI
        // It copies the time control string into eachEngineConfig_.tc for use in createTournament()
        globalSettings_->setTimeControlChangedCallback(
            [this](const ImGuiEngineGlobalSettings::TimeControlSettings& settings) {
                timeControlSettings_ = settings;
            }
        );

        // Callback is triggered when the user selects/deselects engines in the UI
        // It updates the list of engine configurations that will be used in the tournament
        engineSelect_->setConfigurationChangedCallback(
            [this](const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
                engineConfigurations_ = configurations;
            }
        );

        // Poll callback to regularly update tournament data
        pollCallbackHandle_ = StaticCallbacks::poll().registerCallback(
		    [this]() {
    			this->pollData();
		    }
    	);

        // Message callback to handle external messages
        messageCallbackHandle_ = StaticCallbacks::message().registerCallback(
            [this](const std::string& msg) {
                if (msg == "switch_to_tournament_view") {
                    this->activateBoardView(0);
                }
            }
        );
    }

    void TournamentData::setGameManagerPool(const std::shared_ptr<GameManagerPool>& pool) {
        poolAccess_ = GameManagerPoolAccess(pool);
        boardWindowList_.setPoolAccess(poolAccess_);
        imguiConcurrency_->setPoolAccess(poolAccess_);
    }

    void TournamentData::updateTournamentResults() {
        if (tournament_) {
            auto roundSections = tournament_->getSections();
            if (!roundSections.empty()) {
                QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
                    "round", "tournament", roundSections);
            }
        }
    }

    const QaplaTester::TournamentResult& TournamentData::getTournamentResult() const {
        return result_->getResult();
    }

    void TournamentData::setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
        engineConfigurations_ = configurations;
        if (!loadedTournamentData_) {
            loadedTournamentData_ = true;
            loadTournament();
        }
    }

    void TournamentData::loadTournament()
    {
        if (createTournament(false))
        {
            auto sections = QaplaConfiguration::Configuration::instance().getConfigData().
                getSectionList("round", "tournament").value_or(std::vector<QaplaHelpers::IniFile::Section>{});

            if (tournament_)
            {
                tournament_->load(sections);
            }
        }
    }

    std::vector<EngineConfig> TournamentData::getSelectedEngines() const {
        std::vector<EngineConfig> selectedEngines;
        
        for (const auto& tournamentConfig : engineConfigurations_) {
            if (!tournamentConfig.selected) {
                continue;
            }
            auto engine = tournamentConfig.config;
            ImGuiEngineGlobalSettings::applyGlobalConfig(engine, eachEngineConfig_, timeControlSettings_);
            
            selectedEngines.push_back(engine);
        }
        
        return selectedEngines;
    }

    bool TournamentData::createTournament(bool verbose) {
        try {
            if (engineConfigurations_.empty()) {
                throw std::runtime_error("No engines configured");
            }
            if (!tournament_) {
                throw std::runtime_error("Internal Error, Tournament instance not initialized");
            }
            config_->openings = tournamentOpening_->openings();
            if (config_->openings.file.empty()) {
                throw std::runtime_error("No openings file specified.");
            }

            auto selectedEngines = getSelectedEngines();

            PgnSave::tournament().setOptions(pgnConfig());
            QaplaTester::AdjudicationManager::poolInstance().setDrawAdjudicationConfig(tournamentAdjudication_->drawConfig());
            QaplaTester::AdjudicationManager::poolInstance().setResignAdjudicationConfig(tournamentAdjudication_->resignConfig());
            tournament_->createTournament(selectedEngines, *config_);
        } catch (const std::exception& ex) {
            if (verbose) {
                SnackbarManager::instance().showError(std::string("Failed to create tournament: ") + ex.what(),
                    false, "tournament");
            }
            return false;
        }
        return true;
    }

    bool TournamentData::mayStartTournament(bool verbose) {
        if (!tournament_) {
            if (verbose) {
                SnackbarManager::instance().showError("Internal error, tournament not initialized", 
                    false, "tournament");
            }
            return false;
        }
        if (getTotalGames() == 0) {
            if (verbose) {
                if (config_->type == "gauntlet") {
                    SnackbarManager::instance().showError("No games to play in tournament\n"
                        "Did you forget to set at least one engine as gauntlet engine?", 
                        false, "tournament");
                } else {
                    SnackbarManager::instance().showError("No games to play in tournament\n"
                        "You need at least two engines to play a tournament", 
                        false, "tournament");
                }
            }
            return false;
        }
        if (isFinished()) {
            if (verbose) {
                SnackbarManager::instance().showNote("Tournament already finished", 
                    false, "tournament");
            }
            return false;
        }
        return true;
    }

    void TournamentData::startTournament(bool verbose) {
        if (!mayStartTournament(verbose)) {
            return;
        }
        if (!createTournament(true)) {
            return;
        }

        state_ = State::Starting;

        poolAccess_->clearAll();
        tournament_->scheduleAll(0, false, *poolAccess_);
        eloTable_.clear();
        populateEloTable();
        runningTable_.clear();
        populateRunningTable();
        imguiConcurrency_->init();
        imguiConcurrency_->setActive(true);

        if (verbose) {
            SnackbarManager::instance().showSuccess("Tournament started", 
                false, "tournament");
        }
	}

    TournamentConfig& TournamentData::config() {
        return *config_;
    }

    void TournamentData::populateEloTable() {
        eloTable_.clear();

        for (auto scored : result_->getScoredEngines()) {
            std::vector<std::string> row;
            row.push_back(scored.engineName);
            row.push_back(std::format("{:.1f}", scored.elo));
            if (scored.error <= 0) {
                row.emplace_back("-");
            } else {
                row.push_back("+/- " + std::to_string(scored.error));
            }
            row.push_back(scored.formatScore());
            row.push_back(std::format("{:.1f}", scored.getPercentage()));
            row.push_back(std::format("{:.0F}", scored.total));
            eloTable_.push(row);
		}
    }

    void TournamentData::populateMatrixTable() {
        constexpr size_t fixedColumns = 4; // Rank, Engine, Score, %
        matrixTable_.clear();

        const auto& scoredEngines = result_->getScoredEngines();
        if (scoredEngines.empty()) {
            return;
        }

        // Build rows first to determine widest pairwise result for optimal column sizing.
        // Note: ImGuiTable stores rows independently of column definitions - column count
        // is only enforced during rendering, not during data insertion.
        std::string widestResult = buildMatrixTableRows(scoredEngines);

        size_t totalColumns = fixedColumns + scoredEngines.size() + 1; // +1 for S-B column
        matrixTable_.resizeColumns(totalColumns);
        
        float pairwiseColumnWidth = widestResult.empty()
            ? 80.0F
            : matrixTable_.calculateTextWidth(widestResult, 20.0F);
        
        size_t colIndex = fixedColumns; 
        for (const auto& scored : scoredEngines) {
            std::string abbrev = TournamentResultView::abbreviateEngineName(scored.engineName);
            matrixTable_.setColumnHead(colIndex++, { 
                .name = abbrev, 
                .flags = ImGuiTableColumnFlags_WidthFixed, 
                .width = pairwiseColumnWidth, 
                .alignRight = true 
            });
        }
        
        matrixTable_.setColumnHead(colIndex, { .name = "S-B", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F, .alignRight = true });
    }

    std::string TournamentData::buildMatrixTableRows(
        const std::vector<QaplaTester::TournamentResult::Scored>& scoredEngines) {
        
        std::vector<std::string> engineNames;
        engineNames.reserve(scoredEngines.size());
        for (const auto& scored : scoredEngines) {
            engineNames.push_back(scored.engineName);
        }
        
        const auto& tournamentResult = result_->getResult();
        auto duelsMap = TournamentResultView::buildDuelsMap(engineNames, tournamentResult);
        
        std::unordered_map<std::string, double> sbScores;
        TournamentResultView::computeSonnebornBerger(scoredEngines, tournamentResult, sbScores);
        
        std::string widestResult;
        size_t maxLength = 0;
        size_t rank = 1;
        
        for (const auto& scored : scoredEngines) {
            std::vector<std::string> row;
            
            row.push_back(std::format("{:02d}", rank));
            row.push_back(scored.engineName);
            row.push_back(scored.formatScore());
            row.push_back(std::format("{:.1f}", scored.getPercentage()));
            
            for (const auto& opponent : engineNames) {
                std::string result = TournamentResultView::formatPairwiseResult(scored.engineName, opponent, duelsMap);
                if (result.length() > maxLength) {
                    maxLength = result.length();
                    widestResult = result;
                }
                row.push_back(result);
            }
            
            row.push_back(std::format("{:.2f}", sbScores.at(scored.engineName)));
            
            matrixTable_.push(row);
            rank++;
        }
        
        return widestResult;
    }

    void TournamentData::populateCauseTable() {
        std::vector<EngineDuelResult> duelResults;
        
        for (const auto& scored : result_->getScoredEngines()) {
            auto aggregate = scored.result.aggregate(scored.engineName);
            duelResults.push_back(aggregate);
        }
        
        causesTable_.populate(duelResults);
    }

    void TournamentData::populateAdjudicationTable() {
        adjudicationTable_.clear();
        
        auto results = QaplaTester::AdjudicationManager::poolInstance().computeTestResults();

        populateDrawTest(results);
        populateResignTest(results);
    }

    void TournamentData::populateResignTest(QaplaTester::AdjudicationManager::TestResults &results)
    {
        if (results.hasResignTest)
        {
            std::vector<std::string> row;
            for (const auto &entry : results.resignResult)
            {
                if (entry.key == "label" || entry.key == "total" || entry.key == "correct" 
                    || entry.key == "incorrect" || entry.key == "saved" || entry.key == "total_time")
                {
                    row.push_back(entry.value);
                }
            }
            if (!row.empty())
            {
                adjudicationTable_.push(row);
            }
        }
    }

    void TournamentData::populateDrawTest(QaplaTester::AdjudicationManager::TestResults &results)
    {
        if (results.hasDrawTest)
        {
            std::vector<std::string> row;
            for (const auto &entry : results.drawResult)
            {
                if (entry.key == "label" || entry.key == "total" || entry.key == "correct" 
                    || entry.key == "incorrect" || entry.key == "saved" || entry.key == "total_time")
                {
                    row.push_back(entry.value);
                }
            }
            if (!row.empty())
            {
                adjudicationTable_.push(row);
            }
        }
    }

    void TournamentData::populateRunningTable() {
        runningTable_.clear();
        if (tournament_) {
            poolAccess_->withGameRecords(
                [&](const GameRecord& game, [[maybe_unused]] uint32_t gameIndex) {
                    std::vector<std::string> row;
                    row.push_back(game.getWhiteEngineName());
                    row.push_back(game.getBlackEngineName());
                    row.push_back(std::to_string(game.getRound()));
                    row.push_back(std::to_string(game.getGameInRound()));
                    row.push_back(std::to_string(game.getOpeningNo()));
                    runningTable_.push(row);
                    runningCount_++;
                },
            [&]([[maybe_unused]] uint32_t gameIndex) -> bool {
                return true;
            });
            bool anyRunning = boardWindowList_.isAnyRunning();
            if (state_ == State::Starting && anyRunning) {
                state_ = State::Running;
            }
            if (state_ != State::Starting && state_ != State::Stopped && !anyRunning) {
                state_ = State::Stopped;
                SnackbarManager::instance().showSuccess("Tournament finished.", 
                    false, "tournament");
            }
        }
	}

    void TournamentData::pollData() {
        if (tournament_) {
            if (result_->poll(*tournament_, config_->averageElo)) {
                updateTournamentResults();
                populateEloTable();
                populateCauseTable();
                populateAdjudicationTable();
                populateMatrixTable();
            }
            populateRunningTable();
            boardWindowList_.populateViews();
        }
    }

    bool TournamentData::isRunning() const {
        return state_ != State::Stopped;
	}

    bool TournamentData::isAvailable() const {
        return result_ && result_->hasGamesLeft();
    }

    bool TournamentData::hasTasksScheduled() const {
        return tournament_ && tournament_->hasTasksScheduled();
    }

    uint32_t TournamentData::getTotalGames() const {
        if (!config_) {
            return 0;
        }

        std::vector<QaplaTester::EngineConfig> selectedEngines = getSelectedEngines();
        return QaplaTester::Tournament::calculateTotalGames(selectedEngines, *config_);
    }

    bool TournamentData::isFinished() const {
        return getPlayedGames() >= getTotalGames();
    }

    uint32_t TournamentData::getPlayedGames() const {
        if (!result_) {
            return 0;
        }
        return result_->getPlayedGames();
    }

    std::optional<size_t> TournamentData::drawEloTable(const ImVec2& size) {
        if (eloTable_.size() == 0) {
            return std::nullopt;
        }
        return eloTable_.draw(size, true);
    }

    std::optional<size_t> TournamentData::drawMatrixTable(const ImVec2& size) {
        if (matrixTable_.size() == 0) {
            return std::nullopt;
        }
        return matrixTable_.draw(size, true);
    }

    void TournamentData::activateBoardView(size_t gameIndex) {
        if (!tournament_) {
            return;
        }
        const auto& optRow = runningTable_.getRow(gameIndex);
        if (!optRow) {
            return;
        }
        const auto& row = *optRow;
        // Round.Game:WhiteEngine-BlackEngine
        std::string rowId = std::format("{}.{}:{}-{}", row[2], row[3], row[0], row[1]);
        boardWindowList_.setActiveWindowId(rowId);
    }

    std::optional<size_t> TournamentData::drawRunningTable(const ImVec2& size) {
        if (runningTable_.size() == 0) {
            return std::nullopt;
        }
        const auto clickedLine = runningTable_.draw(size, true);
        if (clickedLine) {
            activateBoardView(*clickedLine);
        }
        return clickedLine;
	}

    void TournamentData::drawCauseTable(const ImVec2& size) {
        if (causesTable_.size() == 0) {
            return;
        }
        causesTable_.draw(size);
    }

    void TournamentData::drawAdjudicationTable(const ImVec2& size) {
        if (adjudicationTable_.size() == 0) {
            return;
        }
        adjudicationTable_.draw(size, true);
    }

    void TournamentData::stopPool(bool graceful) {
        // Must be called before deactivating the control so that the pool is informed about 
        // setting concurrency to zero. 
        imguiConcurrency_->update(0);
        // Prevents that concurrency control tells the pool to start new tasks when calculations are stopped
        imguiConcurrency_->setActive(false);

        auto oldState = state_;
        state_ = graceful ? State::GracefulStopping : State::Stopping;
        if (!graceful) {
            // If we are not graceful, we stop all immediately
            poolAccess_->stopAll();
        }
        if (oldState == State::Stopping) {
            SnackbarManager::instance().showNote("Tournament is already stopping.", 
                false, "tournament");
            return;
        }
        if (oldState == State::Stopped) {
            state_ = State::Stopped;
            SnackbarManager::instance().showNote("Tournament is not running.", 
                false, "tournament");
            return;
        }
        if (oldState == State::GracefulStopping && graceful) {
            SnackbarManager::instance().showNote("Tournament is already stopping gracefully.", 
                false, "tournament");
            return;
        }
       
        SnackbarManager::instance().showSuccess(
            graceful ? 
                "Tournament stopped.\nFinishing ongoing games." : 
                "Tournament stopped", 
            false, "tournament"
        );
    }

    void TournamentData::clear(bool verbose) {
        if (!hasTasksScheduled()) {
            if (verbose) {
                SnackbarManager::instance().showNote("Nothing to clear.", 
                    false, "tournament");
            }
            return;
        }
        imguiConcurrency_->setActive(false);
        poolAccess_->clearAll();
        tournament_ = std::make_unique<Tournament>();
        result_ = std::make_unique<TournamentResultIncremental>();
        if (state_ == State::Running) {
            if (verbose) {
                SnackbarManager::instance().showSuccess("Tournament stopped.\nAll results have been cleared.", 
                    false, "tournament");
            }
        } else {
            if (verbose) {
                SnackbarManager::instance().showNote("All results have been cleared.", 
                    false, "tournament");
            }
        }
        state_ = State::Stopped;
    }

    uint32_t TournamentData::getExternalConcurrency() const {
        return imguiConcurrency_->getExternalConcurrency();
    }

    void TournamentData::setExternalConcurrency(uint32_t count) {
        imguiConcurrency_->setExternalConcurrency(count);
    }

    void TournamentData::setPoolConcurrency(uint32_t count, bool nice, bool direct) {
        if (!isRunning()) {
            return;
        }
        imguiConcurrency_->update(count, direct);
        imguiConcurrency_->setNiceStop(nice);
    }

    void TournamentData::loadTournamentConfig() {
        tournamentConfiguration_->loadConfiguration();
    }

    void TournamentData::loadOpenings() {
        tournamentOpening_->loadConfiguration();
        config_->openings = tournamentOpening_->openings();
    }

    void TournamentData::loadPgnConfig() {
        tournamentPgn_->loadConfiguration();
    }

     void TournamentData::loadEngineSelectionConfig() {
        auto sections = QaplaConfiguration::Configuration::instance()
            .getConfigData().getSectionList("engineselection", "tournament")
            .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
        engineSelect_->setId("tournament");
        engineSelect_->setEnginesConfiguration(sections);
    }

    void TournamentData::loadGlobalSettingsConfig() {
        auto& config = QaplaConfiguration::Configuration::instance();
        
        // Load global engine settings
        auto globalSections = config.getConfigData().getSectionList("eachengine", "tournament")
            .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
        globalSettings_->setId("tournament");
        globalSettings_->setGlobalConfiguration(globalSections);
        
        // Load time control settings
        auto timeControlSections = config.getConfigData().getSectionList("timecontroloptions", "tournament")
            .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
        globalSettings_->setTimeControlConfiguration(timeControlSections);
    }

    void TournamentData::loadConfig() {
        loadTournamentConfig();
        loadOpenings();
        loadPgnConfig();
        tournamentAdjudication_->loadConfiguration();
        loadEngineSelectionConfig();
        loadGlobalSettingsConfig();
    }

    void TournamentData::saveTournament(const std::string& filename) {
        if (filename.empty()) {
            SnackbarManager::instance().showError("No filename specified for saving tournament.", 
                false, "tournament");
            return;
        }

        try {
            std::ofstream out(filename, std::ios::trunc);
            if (!out) {
                SnackbarManager::instance().showError("Failed to open file for writing: " + filename, 
                    false, "tournament");
                return;
            }

            auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();

            // Save each section type (all with id "tournament")
            for (const auto& sectionName : sectionNames) {
                auto sections = configData.getSectionList(sectionName, "tournament");
                if (sections && !sections->empty()) {
                    for (const auto& section : *sections) {
                        QaplaHelpers::IniFile::saveSection(out, section);
                    }
                }
            }

            out.close();
            if (!out) {
                SnackbarManager::instance().showError("Error while writing to file: " + filename,
                    false, "tournament");
                return;
            }

            SnackbarManager::instance().showSuccess("Tournament saved to: " + filename, 
                false, "tournament");
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showError("Failed to save tournament: " + std::string(e.what()), 
                false, "tournament");
        }
    }

    void TournamentData::loadTournament(const std::string& filename) {
        if (filename.empty()) {
            SnackbarManager::instance().showError("No filename specified for loading tournament.", 
                false, "tournament");
            return;
        }

        try {
            std::ifstream in(filename);
            if (!in) {
                SnackbarManager::instance().showError("Failed to open file for reading: " + filename, 
                    false, "tournament");
                return;
            }

            // Load all sections from the file
            QaplaHelpers::ConfigData configData;
            configData.load(in);
            in.close();

            // Transfer sections to the global configuration
            for (const auto& sectionName : sectionNames) {
                auto sections = configData.getSectionList(sectionName, "tournament");
                if (sections && !sections->empty()) {
                    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
                        sectionName, "tournament", *sections);
                }
            }

            // Reload configuration from the updated singleton
            loadConfig();

            // Create tournament based on the configuration and load the tournament data
            loadTournament();

            SnackbarManager::instance().showSuccess("Tournament loaded from: " + filename, 
                false, "tournament");
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showError("Failed to load tournament: " + std::string(e.what()), 
                false, "tournament");
        }
    }

}

