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
#include "tournament-board-window.h"
#include "snackbar.h"
#include "imgui-concurrency.h"
#include "configuration.h"

#include "qapla-tester/string-helper.h"

#include "qapla-tester/engine-option.h"
#include "qapla-tester/engine-worker-factory.h"

#include "qapla-tester/tournament.h"
#include "qapla-tester/tournament-result.h"

#include "qapla-tester/game-manager-pool.h"
#include "qapla-tester/pgn-io.h"
#include "qapla-tester/adjudication-manager.h"
#include "imgui-table.h"

#include <imgui.h>

namespace QaplaWindows {

    TournamentData::TournamentData() : 
        tournament_(std::make_unique<Tournament>()),
		config_(std::make_unique<TournamentConfig>()),
        result_(std::make_unique<TournamentResultIncremental>()),
        imguiConcurrency_(std::make_unique<ImGuiConcurrency>()),
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
        causeTable_(
            "Causes",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Name", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "WDL", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F },
                { .name = "Count", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Cause", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 200.0F }
            }
        )
    { 
        runningTable_.setClickable(true);
        loadConfig();
    }

    TournamentData::~TournamentData() = default;

    void TournamentData::updateConfiguration() {
        // Each Engine Config
         QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "eachengine", "tournament", {{
                .name = "eachengine",
                .entries = QaplaHelpers::IniFile::KeyValueMap{
                    {"id", "tournament"},
                    {"tc", eachEngineConfig_.tc},
                    {"restart", eachEngineConfig_.restart},
                    {"trace", eachEngineConfig_.traceLevel},
                    {"ponder", eachEngineConfig_.ponder},
                    {"hash", std::to_string(eachEngineConfig_.hash)}
                }
        }});

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

        // Opening Config
        QaplaHelpers::IniFile::KeyValueMap openingEntries{
            {"id", "tournament"},
            {"file", config_->openings.file},
            {"format", config_->openings.format},
            {"order", config_->openings.order},
            {"seed", std::to_string(config_->openings.seed)},
            {"start", std::to_string(config_->openings.start)},
            {"policy", config_->openings.policy}
        };

        if (config_->openings.plies) {
            openingEntries.emplace_back("plies", std::to_string(*config_->openings.plies));
        }
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "opening", "tournament", {{
                .name = "opening",
                .entries = openingEntries
        }});

        // PGN Config
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "pgnoutput", "tournament", {{
                .name = "pgnoutput",
                .entries = QaplaHelpers::IniFile::KeyValueMap{
                    {"id", "tournament"},
                    {"file", pgnConfig_.file},
                    {"append", pgnConfig_.append ? "true" : "false"},
                    {"onlyFinishedGames", pgnConfig_.onlyFinishedGames ? "true" : "false"},
                    {"minimalTags", pgnConfig_.minimalTags ? "true" : "false"},
                    {"saveAfterMove", pgnConfig_.saveAfterMove ? "true" : "false"},
                    {"includeClock", pgnConfig_.includeClock ? "true" : "false"},
                    {"includeEval", pgnConfig_.includeEval ? "true" : "false"},
                    {"includePv", pgnConfig_.includePv ? "true" : "false"},
                    {"includeDepth", pgnConfig_.includeDepth ? "true" : "false"}
                }
        }});

        // Draw Adjudication Config
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "drawadjudication", "tournament", {{
                .name = "drawadjudication",
                .entries = QaplaHelpers::IniFile::KeyValueMap{
                    {"id", "tournament"},
                    {"minFullMoves", std::to_string(drawConfig_.minFullMoves)},
                    {"requiredConsecutiveMoves", std::to_string(drawConfig_.requiredConsecutiveMoves)},
                    {"centipawnThreshold", std::to_string(drawConfig_.centipawnThreshold)},
                    {"testOnly", drawConfig_.testOnly ? "true" : "false"}
                }
        }});

        // Resign Adjudication Config
        QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
            "resignadjudication", "tournament", {{
                .name = "resignadjudication",
                .entries = QaplaHelpers::IniFile::KeyValueMap{
                    {"id", "tournament"},
                    {"requiredConsecutiveMoves", std::to_string(resignConfig_.requiredConsecutiveMoves)},
                    {"centipawnThreshold", std::to_string(resignConfig_.centipawnThreshold)},
                    {"twoSided", resignConfig_.twoSided ? "true" : "false"},
                    {"testOnly", resignConfig_.testOnly ? "true" : "false"}
                }
        }});

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

    bool TournamentData::createTournament(bool verbose) {
        if (engineConfigurations_.empty()) {
            if (verbose) {
                SnackbarManager::instance().showError("No engines configured for the tournament.");
            }
            return false;
		}
        try {
            std::vector<EngineConfig> selectedEngines;
            config_->type = "round-robin";
            for (auto& tournamentConfig : engineConfigurations_) {
                if (!tournamentConfig.selected) {
                    continue;
                }
                EngineConfig engine = tournamentConfig.config;
                if (eachEngineConfig_.ponder != "per engine") {
                    engine.setPonder(eachEngineConfig_.ponder == "on");
                }
                engine.setTimeControl(eachEngineConfig_.tc);
                engine.setRestartOption(parseRestartOption(eachEngineConfig_.restart));
                engine.setTraceLevel(eachEngineConfig_.traceLevel);
                engine.setOptionValue("Hash", std::to_string(eachEngineConfig_.hash));
                if (engine.gauntlet()) {
                    config_->type = "gauntlet";
                }
                selectedEngines.push_back(engine);
            }

            if (!validateOpenings()) {
                return false;
            }

            if (tournament_) {
                PgnIO::tournament().setOptions(pgnConfig_);
                AdjudicationManager::instance().setDrawAdjudicationConfig(drawConfig_);
                AdjudicationManager::instance().setResignAdjudicationConfig(resignConfig_);
                tournament_->createTournament(selectedEngines, *config_);
            } else {
                SnackbarManager::instance().showError("Internal error, tournament not initialized");
                return false;
            }
        } catch (const std::exception& ex) {
            SnackbarManager::instance().showError(std::string("Failed to create tournament: ") + ex.what());
            return false;
        }
        return true;
    }

    void TournamentData::startTournament() {
        if (!tournament_) {
            SnackbarManager::instance().showError("Internal error, tournament not initialized");
            return;
        }
        if (!createTournament(true)) {
            return;
        }

        state_ = State::Starting;

        GameManagerPool::getInstance().clearAll();
        result_->setGamesLeft();
        state_ = State::Starting;
        tournament_->scheduleAll(0, false);
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

    static void addRow(ImGuiTable& table, const std::string& name, const std::string& wdl, const std::string& cause, int count) {
        if (count == 0) {
            return;
        }
        std::vector<std::string> row;
        row.push_back(name);
        row.push_back(wdl);
        row.push_back(std::to_string(count));
        row.push_back(cause);
        table.push(row);
    }

    void TournamentData::populateCauseTable() {
        causeTable_.clear();
        for (const auto& scored : result_->getScoredEngines()) {
            auto aggregate = scored.result.aggregate(scored.engineName);
            for (uint32_t index = 0; index < aggregate.causeStats.size(); index++) {
                const auto& stat = aggregate.causeStats[index];
                addRow(causeTable_, scored.engineName, "win", to_string(static_cast<GameEndCause>(index)), stat.win);
            }
            for (uint32_t index = 0; index < aggregate.causeStats.size(); index++) {
                const auto& stat = aggregate.causeStats[index];
                addRow(causeTable_, scored.engineName, "draw", to_string(static_cast<GameEndCause>(index)), stat.draw);
            }
            for (uint32_t index = 0; index < aggregate.causeStats.size(); index++) {
                const auto& stat = aggregate.causeStats[index];
                addRow(causeTable_, scored.engineName, "loss", to_string(static_cast<GameEndCause>(index)), stat.loss);
            }
        }
    }

    void TournamentData::populateRunningTable() {
        runningTable_.clear();
        if (tournament_) {
            for (auto& window: boardWindow_) {
                window.setRunning(false);
            }
            bool anyRunning = false;
            GameManagerPool::getInstance().withGameRecords(
                [&](const GameRecord& game, uint32_t gameIndex) {
                    std::vector<std::string> row;
                    row.push_back(game.getWhiteEngineName());
                    row.push_back(game.getBlackEngineName());
                    row.push_back(std::to_string(game.getRound()));
                    row.push_back(std::to_string(game.getGameInRound()));
                    row.push_back(std::to_string(game.getOpeningNo()));
                    runningTable_.push(row);
                    runningCount_++;
                    if (gameIndex < boardWindow_.size()) {
                        boardWindow_[gameIndex].setRunning(true);
                        anyRunning = true;
                    }
                },
            [&](uint32_t gameIndex) -> bool {
                return true;
            });
            if (state_ == State::Starting && anyRunning) {
                state_ = State::Running;
            }
            if (state_ != State::Starting && !anyRunning) {
                state_ = State::Stopped;
            }
        }
	}

    void TournamentData::populateViews() {
        if (tournament_) {
            GameManagerPool::getInstance().withGameRecords(
                [&](const GameRecord& game, uint32_t gameIndex) {
                    if (gameIndex >= boardWindow_.size()) {
                        return;
                    }
                    boardWindow_[gameIndex].setFromGameRecord(game);
                },
                [&](uint32_t gameIndex) -> bool {
                    while (gameIndex >= boardWindow_.size()) {
                        TournamentBoardWindow window;
                        boardWindow_.push_back(std::move(window));
                    }
                    return true;
                }
            );
            GameManagerPool::getInstance().withEngineRecords(
                [&](const EngineRecords& records, uint32_t gameIndex) {
                    if (gameIndex >= boardWindow_.size()) {
                        return;
                    }
                    boardWindow_[gameIndex].setFromEngineRecords(records);
                },
                [&](uint32_t gameIndex) -> bool {
                    if (gameIndex >= boardWindow_.size()) {
                        return false;
                    }
                    return boardWindow_[gameIndex].isActive();
                }
            );
            GameManagerPool::getInstance().withMoveRecord(
                [&](const MoveRecord& record, uint32_t gameIndex, uint32_t playerIndex) 
                {
                    // Called for each player in the board, playerIndex == 0 indicates the next board.
                    if (gameIndex >= boardWindow_.size()) { 
                        return;
                    }

                    boardWindow_[gameIndex].setFromMoveRecord(record, playerIndex);
                },
                [&](uint32_t gameIndex) -> bool {
                    if (gameIndex >= boardWindow_.size()) {
                        return false;
                    }
                    return boardWindow_[gameIndex].isActive();
                }
            );
        }
    }

    void TournamentData::pollData() {
        if (tournament_) {
            if (result_->poll(*tournament_, config_->averageElo)) {
                updateTournamentResults();
                populateEloTable();
                populateCauseTable();
            }
            populateRunningTable();
            populateViews();
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
        if (causeTable_.size() == 0) {
            return;
        }
        causeTable_.draw(size, true);
    }

    void TournamentData::drawTabs() {
        int32_t newIndex = -1;
        for (int32_t index = 0; index < boardWindow_.size(); index++) {
            auto& window = boardWindow_[index];
            std::string tabName = window.id();
            std::string tabId = "###Game" + std::to_string(index);
            if (!window.isRunning() && index != selectedIndex_) {
                continue;
            }
            if (ImGui::BeginTabItem((tabName + tabId).c_str())) {
                if (window.isActive()) {
                    window.draw();
                } else if (selectedIndex_ >= 0 && std::cmp_equal(selectedIndex_, boardWindow_.size())) {
                    boardWindow_[selectedIndex_].draw();
                }
                window.setActive(true);
                newIndex = index;
                ImGui::EndTabItem();
            } else {
                window.setActive(false);
            }
        }
        selectedIndex_ = newIndex;
    }

    void TournamentData::stopPool(bool graceful) {
        imguiConcurrency_->update(0);
        imguiConcurrency_->setActive(false);
        auto oldState = state_;
        state_ = graceful ? State::GracefulStopping : State::Stopped;
        if (!graceful) {
            GameManagerPool::getInstance().stopAll();
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
        state_ = State::Stopped;
        GameManagerPool::getInstance().clearAll();
        tournament_ = std::make_unique<Tournament>();
        result_ = std::make_unique<TournamentResultIncremental>();
        SnackbarManager::instance().showSuccess("Tournament stopped.\nAll results have been cleared.");
    }

    void TournamentData::setPoolConcurrency(uint32_t count, bool nice) {
        if (!isRunning()) {
            return;
        }
        imguiConcurrency_->setNiceStop(nice);
        imguiConcurrency_->update(count);
    }

    void TournamentData::loadEachEngineConfig() {
        auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
        auto sections = configData.getSectionList("eachengine", "tournament");
        if (!sections || sections->empty()) {
            return;
        }

        for (const auto& [key, value] : (*sections)[0].entries) {
            if (key == "tc") {
                eachEngineConfig_.tc = value;
                if (value.empty()) {
                    eachEngineConfig_.tc = "60+0";
                }
            }
            else if (key == "restart") {
                eachEngineConfig_.restart = value;
                if (value != "auto" && value != "on" && value != "off") {
                    eachEngineConfig_.restart = "auto";
                }
            }
            else if (key == "trace") {
                eachEngineConfig_.traceLevel = value;
                if (value != "command" && value != "all" && value != "none") {
                    eachEngineConfig_.traceLevel = "none";
				}
            }
            else if (key == "ponder") {
                eachEngineConfig_.ponder = value;
            }
            else if (key == "hash") {
                eachEngineConfig_.hash = QaplaHelpers::to_uint32(value).value_or(32);
            }
        }
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
        auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
        auto sections = configData.getSectionList("opening", "tournament");
        if (!sections || sections->empty()) {
            return;
        }

        for (const auto& [key, value] : (*sections)[0].entries) {
            if (key == "file") {
                config_->openings.file = value;
            }
            else if (key == "format" && (value == "pgn" || value == "epd" || value == "raw")) {
                config_->openings.format = value;
            }
            else if (key == "order" && (value == "sequential" || value == "random")) {
                config_->openings.order = value;
            }
            else if (key == "seed") {
                config_->openings.seed = QaplaHelpers::to_uint32(value).value_or(815);
            }
            else if (key == "plies") {
                config_->openings.plies = QaplaHelpers::to_int(value);
            }
            else if (key == "start") {
                config_->openings.start = QaplaHelpers::to_uint32(value).value_or(0);
            }
            else if (key == "policy" && (value == "default" || value == "encounter" || value == "round")) {
                config_->openings.policy = value;
            }
        }
    }

    void TournamentData::loadPgnConfig() {
        auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
        auto sections = configData.getSectionList("pgnoutput", "tournament");
        if (!sections || sections->empty()) {
            return;
        }

        for (const auto& [key, value] : (*sections)[0].entries) {
            if (key == "file") {
                pgnConfig_.file = value;
            }
            else if (key == "append") {
                pgnConfig_.append = (value == "true");
            }
            else if (key == "onlyFinishedGames") {
                pgnConfig_.onlyFinishedGames = (value == "true");
            }
            else if (key == "minimalTags") {
                pgnConfig_.minimalTags = (value == "true");
            }
            else if (key == "saveAfterMove") {
                pgnConfig_.saveAfterMove = (value == "true");
            }
            else if (key == "includeClock") {
                pgnConfig_.includeClock = (value == "true");
            }
            else if (key == "includeEval") {
                pgnConfig_.includeEval = (value == "true");
            }
            else if (key == "includePv") {
                pgnConfig_.includePv = (value == "true");
            }
            else if (key == "includeDepth") {
                pgnConfig_.includeDepth = (value == "true");
            }
        }
    }

    void TournamentData::loadDrawAdjudicationConfig() {
        auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
        auto sections = configData.getSectionList("drawadjudication", "tournament");
        if (!sections || sections->empty()) {
            return;
        }

        for (const auto& [key, value] : (*sections)[0].entries) {
            if (key == "minFullMoves") {
                drawConfig_.minFullMoves = QaplaHelpers::to_uint32(value).value_or(0);
            }
            else if (key == "requiredConsecutiveMoves") {
                drawConfig_.requiredConsecutiveMoves = QaplaHelpers::to_uint32(value).value_or(0);
            }
            else if (key == "centipawnThreshold") {
                drawConfig_.centipawnThreshold = QaplaHelpers::to_int(value).value_or(0);
            }
            else if (key == "testOnly") {
                drawConfig_.testOnly = (value == "true");
            }
        }
    }

    void TournamentData::loadResignAdjudicationConfig() {
        auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
        auto sections = configData.getSectionList("resignadjudication", "tournament");
        if (!sections || sections->empty()) {
            return;
        }

        for (const auto& [key, value] : (*sections)[0].entries) {
            if (key == "requiredConsecutiveMoves") {
                resignConfig_.requiredConsecutiveMoves = QaplaHelpers::to_uint32(value).value_or(0);
            }
            else if (key == "centipawnThreshold") {
                resignConfig_.centipawnThreshold = QaplaHelpers::to_int(value).value_or(0);
            }
            else if (key == "twoSided") {
                resignConfig_.twoSided = (value == "true");
            }
            else if (key == "testOnly") {
                resignConfig_.testOnly = (value == "true");
            }
        }
    }

    bool TournamentData::validateOpenings() {
        bool valid = true;
        if (config_->openings.file.empty()) {
            SnackbarManager::instance().showError("No openings file specified.");
            valid = false;
        }
        return valid;
	}

    void TournamentData::loadConfig() {
        loadEachEngineConfig();
        loadTournamentConfig();
        loadOpenings();
        loadPgnConfig();
        loadDrawAdjudicationConfig();
        loadResignAdjudicationConfig();
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

