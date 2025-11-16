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
#include "adjudication-manager.h"
#include "imgui-table.h"

#include <imgui.h>



using namespace QaplaTester;

namespace QaplaWindows {

    TournamentData::TournamentData() : 
        boardWindowList_("Tournament"),
        tournament_(std::make_unique<Tournament>()),
		config_(std::make_unique<TournamentConfig>()),
        result_(std::make_unique<TournamentResultIncremental>()),
        imguiConcurrency_(std::make_unique<ImGuiConcurrency>()),
        engineSelect_(std::make_unique<ImGuiEngineSelect>()),
        globalSettings_(std::make_unique<ImGuiEngineGlobalSettings>()),
        tournamentOpening_(std::make_unique<ImGuiTournamentOpening>()),
        tournamentPgn_(std::make_unique<ImGuiTournamentPgn>()),
        tournamentAdjudication_(std::make_unique<ImGuiTournamentAdjudication>()),
        eloTable_(
            "TournamentResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Name", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Elo", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Error", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Score", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
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
    }

    void TournamentData::setGameManagerPool(const std::shared_ptr<GameManagerPool>& pool) {
        poolAccess_ = GameManagerPoolAccess(pool);
        boardWindowList_.setPoolAccess(poolAccess_);
        imguiConcurrency_->setPoolAccess(poolAccess_);
    }

    void TournamentData::updateConfiguration() {

        // Tournament Config
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "tournament", "tournament", {{
                .name = "tournament",
                .entries = QaplaHelpers::IniFile::KeyValueMap{
                    {"id", "tournament"},
                    {"event", config_->event},
                    {"type", config_->type},
                    {"rounds", std::to_string(config_->rounds)},
                    {"games", std::to_string(config_->games)},
                    {"repeat", std::to_string(config_->repeat)},
                    {"noSwap", config_->noSwap ? "true" : "false"},
                    {"averageElo", std::to_string(config_->averageElo)},
                    {"saveInterval", std::to_string(config_->saveInterval)}
                }
        }});

        auto openingSections = tournamentOpening_->getSections();
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "opening", "tournament", openingSections);
        
        // PGN Config
        auto pgnSections = tournamentPgn_->getSections();
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "pgnoutput", "tournament", pgnSections);

        // Adjudication Config
        auto adjudicationSections = tournamentAdjudication_->getSections();
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "drawadjudication", "tournament", { adjudicationSections[0] });
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "resignadjudication", "tournament", { adjudicationSections[1] });

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

            PgnIO::tournament().setOptions(pgnConfig());
            QaplaTester::AdjudicationManager::poolInstance().setDrawAdjudicationConfig(tournamentAdjudication_->drawConfig());
            QaplaTester::AdjudicationManager::poolInstance().setResignAdjudicationConfig(tournamentAdjudication_->resignConfig());
            tournament_->createTournament(selectedEngines, *config_);
        } catch (const std::exception& ex) {
            if (verbose) {
                SnackbarManager::instance().showError(std::string("Failed to create tournament: ") + ex.what());
            }
            return false;
        }
        return true;
    }

    void TournamentData::startTournament() {
        if (!tournament_) {
            SnackbarManager::instance().showError("Internal error, tournament not initialized");
            return;
        }
        if (isFinished()) {
            SnackbarManager::instance().showNote("Tournament already finished");
            return;
        }
        if (!createTournament(true)) {
            return;
        }


        state_ = State::Starting;

        poolAccess_->clearAll();
        // Result is incremental and does not support incremental handling of changed tournaments
        // This will not delete results, but force the incremental result to reprocess all results
        result_->clear();
        
        state_ = State::Starting;
        tournament_->scheduleAll(0, false, *poolAccess_);
        eloTable_.clear();
        populateEloTable();
        runningTable_.clear();
        populateRunningTable();
        imguiConcurrency_->init();
        imguiConcurrency_->setActive(true);
        state_ = State::Starting;
		SnackbarManager::instance().showSuccess("Tournament started");
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
            row.push_back(std::format("{:.1f}%", scored.score * 100.0));
            row.push_back(std::format("{:.0F}", scored.total));
            eloTable_.push(row);
		}
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
                if (entry.key == "label")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "total")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "correct")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "incorrect")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "saved")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "total_time")
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
                if (entry.key == "label")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "total")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "correct")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "incorrect")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "saved")
                {
                    row.push_back(entry.value);
                }
                else if (entry.key == "total_time")
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
                SnackbarManager::instance().showSuccess("Tournament finished.");
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

    std::optional<size_t> TournamentData::drawRunningTable(const ImVec2& size) {
        if (runningTable_.size() == 0) {
            return std::nullopt;
        }
        return runningTable_.draw(size, true);
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
        state_ = graceful ? State::GracefulStopping : State::Stopped;
        if (!graceful) {
            // If we are not graceful, we stop all immediately
            poolAccess_->stopAll();
        }

        if (oldState == State::Stopped) {
            SnackbarManager::instance().showNote("Tournament is not running.");
            return;
        }
        if (oldState == State::GracefulStopping && graceful) {
            SnackbarManager::instance().showNote("Tournament is already stopping gracefully.");
            return;
        }
       
        SnackbarManager::instance().showSuccess(
            graceful ? 
                "Tournament stopped.\nFinishing ongoing games." : 
                "Tournament stopped"
        );
    }

    void TournamentData::clear() {
        if (!hasTasksScheduled()) {
            SnackbarManager::instance().showNote("Nothing to clear.");
            return;
        }
        imguiConcurrency_->setActive(false);
        poolAccess_->clearAll();
        tournament_ = std::make_unique<Tournament>();
        result_ = std::make_unique<TournamentResultIncremental>();
        if (state_ == State::Running) {
            SnackbarManager::instance().showSuccess("Tournament stopped.\nAll results have been cleared.");
        } else {
            SnackbarManager::instance().showNote("All results have been cleared.");
        }
        state_ = State::Stopped;
    }

    void TournamentData::setPoolConcurrency(uint32_t count, bool nice) {
        if (!isRunning()) {
            return;
        }
        imguiConcurrency_->setNiceStop(nice);
        imguiConcurrency_->update(count);
    }
    
    void TournamentData::loadTournamentConfig() {
        auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
        auto sections = configData.getSectionList("tournament", "tournament");
        if (!sections || sections->empty()) {
            return;
        }

        for (const auto& [key, value] : (*sections)[0].entries) {
            if (key == "event") {
                config_->event = value;
            }
            else if (key == "type") {
                config_->type = value;
            }
            else if (key == "rounds") {
                config_->rounds = QaplaHelpers::to_uint32(value).value_or(1);
            }
            else if (key == "games") {
                config_->games = QaplaHelpers::to_uint32(value).value_or(1);
            }
            else if (key == "repeat") {
                config_->repeat = QaplaHelpers::to_uint32(value).value_or(1);
            }
            else if (key == "noSwap") {
                config_->noSwap = (value == "true");
            }
            else if (key == "averageElo") {
                config_->averageElo = QaplaHelpers::to_int(value).value_or(0);
            }
            else if (key == "saveInterval") {
                config_->saveInterval = QaplaHelpers::to_uint32(value).value_or(10);
            }
        }
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
            SnackbarManager::instance().showError("No filename specified for saving tournament.");
            return;
        }

        try {
            std::ofstream out(filename, std::ios::trunc);
            if (!out) {
                SnackbarManager::instance().showError("Failed to open file for writing: " + filename);
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
                SnackbarManager::instance().showError("Error while writing to file: " + filename);
                return;
            }

            SnackbarManager::instance().showSuccess("Tournament saved to: " + filename);
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showError("Failed to save tournament: " + std::string(e.what()));
        }
    }

    void TournamentData::loadTournament(const std::string& filename) {
        if (filename.empty()) {
            SnackbarManager::instance().showError("No filename specified for loading tournament.");
            return;
        }

        try {
            std::ifstream in(filename);
            if (!in) {
                SnackbarManager::instance().showError("Failed to open file for reading: " + filename);
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

            SnackbarManager::instance().showSuccess("Tournament loaded from: " + filename);
        }
        catch (const std::exception& e) {
            SnackbarManager::instance().showError("Failed to load tournament: " + std::string(e.what()));
        }
    }

}

