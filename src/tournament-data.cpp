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
                { "Name", ImGuiTableColumnFlags_WidthFixed, 150.0f },
                { "Elo", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Error", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Score", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Total", ImGuiTableColumnFlags_WidthFixed, 50.0f, true }
            }
        ),
        runningTable_(
            "Running",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { "White", ImGuiTableColumnFlags_WidthFixed, 150.0f },
                { "Black", ImGuiTableColumnFlags_WidthFixed, 150.0f },
                { "Round", ImGuiTableColumnFlags_WidthFixed, 50.0f },
                { "Game", ImGuiTableColumnFlags_WidthFixed, 50.0f },
                { "Opening", ImGuiTableColumnFlags_WidthFixed, 50.0f }
            }
		),
        causeTable_(
            "Causes",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { "Name", ImGuiTableColumnFlags_WidthFixed, 150.0f },
                { "WDL", ImGuiTableColumnFlags_WidthFixed, 50.0f },
                { "Count", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Cause", ImGuiTableColumnFlags_WidthFixed, 200.0f }
            }
        )
    { 
        runningTable_.setClickable(true);
        init();
    }

    TournamentData::~TournamentData() = default;

    void TournamentData:: init() {
    }

    bool TournamentData::createTournament(bool verbose) {
        if (engineConfigurations_.empty()) {
            if (verbose) {
                SnackbarManager::instance().showError("No engines configured for the tournament.");
            }
            return false;
		}
        std::vector<EngineConfig> selectedEngines;
        config_->type = "round-robin";
        for (auto& tournamentConfig : engineConfigurations_) {
            if (!tournamentConfig.selected) continue;
            EngineConfig engine = tournamentConfig.config;
            engine.setPonder(eachEngineConfig_.ponder);
			engine.setTimeControl(eachEngineConfig_.tc);
			engine.setRestartOption(parseRestartOption(eachEngineConfig_.restart));
			engine.setTraceLevel(eachEngineConfig_.traceLevel);
            engine.setOptionValue("Hash", std::to_string(eachEngineConfig_.hash));
            if (engine.gauntlet()) {
                config_->type = "gauntlet";
            }
            selectedEngines.push_back(engine);
		}

        if (!validateOpenings()) return false;

        if (tournament_) {
            PgnIO::tournament().setOptions(pgnConfig_);
            AdjudicationManager::instance().setDrawAdjudicationConfig(drawConfig_);
            AdjudicationManager::instance().setResignAdjudicationConfig(resignConfig_);
			tournament_->createTournament(selectedEngines, *config_);
        } else {
            SnackbarManager::instance().showError("Internal error, tournament not initialized");
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
                row.push_back("-");
            } else {
                row.push_back("+/- " + std::to_string(scored.error));
            }
            row.push_back(std::format("{:.1f}%", scored.score * 100.0));
            row.push_back(std::format("{:.0f}", scored.total));
            eloTable_.push(row);
		}
    }

    static void addRow(ImGuiTable& table, const std::string& name, const std::string& wdl, const std::string& cause, int count) {
        if (count == 0) return;
        std::vector<std::string> row;
        row.push_back(name);
        row.push_back(wdl);
        row.push_back(std::to_string(count));
        row.push_back(cause);
        table.push(row);
    }

    void TournamentData::populateCauseTable() {
        causeTable_.clear();
        for (auto scored : result_->getScoredEngines()) {
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
                [&](const GameRecord& game, uint32_t gameIndex
            ) {
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
                    if (gameIndex >= boardWindow_.size()) return;
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
                    if (gameIndex >= boardWindow_.size()) return;
                    boardWindow_[gameIndex].setFromEngineRecords(records);
                },
                [&](uint32_t gameIndex) -> bool {
                    if (gameIndex >= boardWindow_.size()) return false;
                    return boardWindow_[gameIndex].isActive();
                }
            );
            GameManagerPool::getInstance().withMoveRecord(
                [&](const MoveRecord& record, uint32_t gameIndex, uint32_t playerIndex) 
                {
                    // Called for each player in the board, playerIndex == 0 indicates the next board.
                    if (gameIndex >= boardWindow_.size()) return;
                    boardWindow_[gameIndex].setFromMoveRecord(record, playerIndex);
                },
                [&](uint32_t gameIndex) -> bool {
                    if (gameIndex >= boardWindow_.size()) return false;
                    return boardWindow_[gameIndex].isActive();
                }
            );
        }
    }

    void TournamentData::pollData() {
        if (tournament_) {
            if (result_->poll(*tournament_, config_->averageElo)) {
                QaplaConfiguration::Configuration::instance().setModified();
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

    std::optional<size_t> TournamentData::drawEloTable(const ImVec2& size) const {
        if (eloTable_.size() == 0) return std::nullopt;
        return eloTable_.draw(size, true);
    }

    std::optional<size_t> TournamentData::drawRunningTable(const ImVec2& size) const {
        if (runningTable_.size() == 0) return std::nullopt;
        return runningTable_.draw(size, true);
	}

    void TournamentData::drawCauseTable(const ImVec2& size) const {
        if (causeTable_.size() == 0) return;
        causeTable_.draw(size, true);
    }

    void TournamentData::drawTabs() {
        int32_t newIndex = -1;
        for (int32_t index = 0; index < boardWindow_.size(); index++) {
            auto& window = boardWindow_[index];
            std::string tabName = "Game " + window.id();
            std::string tabId = "###Game" + std::to_string(index);
            if (!window.isRunning() && index != selectedIndex_) continue;
            if (ImGui::BeginTabItem((tabName + tabId).c_str())) {
                if (window.isActive()) {
                    window.draw();
                } else if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int32_t>(boardWindow_.size())) {
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
        if (!graceful) GameManagerPool::getInstance().stopAll();

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
        if (!isRunning()) return;
        imguiConcurrency_->setNiceStop(nice);
        imguiConcurrency_->update(count);
    }

    void TournamentData::saveTournamentEngines(std::ostream& out, const std::string& header) const {
        for (const auto& engine : engineConfigurations_) {
            out << "[" << header << "]\n";
            out << "name" << "=" << engine.config.getName() << "\n";
            out << "selected=" << (engine.selected ? "true" : "false") << "\n";
            out << "gauntlet=" << (engine.config.isGauntlet() ? "true" : "false") << "\n";
            out << "\n";
        }
    }

    void TournamentData::loadTournamentEngine(const QaplaHelpers::IniFile::KeyValueMap keyValue) {
        TournamentEngineConfig engine;
        std::string name;
        for (const auto& [key, value] : keyValue) {
            if (key == "selected") {
                engine.selected = (value == "true");
            } else if (key == "name") {
                name = value;
                auto config = EngineWorkerFactory::getConfigManager().getConfig(name);
                if (config) {
                    engine.config = *config;
                }
            } else if (key == "gauntlet") {
                engine.config.setGauntlet(value == "true");
            }
        }
        engineConfigurations_.push_back(engine);
    }

    void TournamentData::saveEachEngineConfig(std::ostream& out, const std::string& header) const {
        out << "[" << header << "]\n";

        out << "tc=" << eachEngineConfig_.tc << "\n";
        out << "restart=" << eachEngineConfig_.restart << "\n";
		out << "trace=" << eachEngineConfig_.traceLevel << "\n";
		out << "ponder=" << (eachEngineConfig_.ponder ? "true" : "false") << "\n";
		out << "hash=" << eachEngineConfig_.hash << "\n";
        out << "\n";
    }

    void TournamentData::loadEachEngineConfig(const QaplaHelpers::IniFile::KeyValueMap keyValue) {
        for (const auto& [key, value] : keyValue) {
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
                eachEngineConfig_.ponder = (value == "true");
            }
            else if (key == "hash") {
                try {
                    eachEngineConfig_.hash = std::stoul(value);
                } catch (...) {
                    eachEngineConfig_.hash = 32;
                } 
            }
        }
    }

    void TournamentData::saveTournamentConfig(std::ostream& out, const std::string& header) const {
        out << "[" << header << "]\n";

        // Save only fields edited in the tournament window
        out << "event=" << config_->event << "\n";
        out << "type=" << config_->type << "\n";
        out << "rounds=" << config_->rounds << "\n";
        out << "games=" << config_->games << "\n";
        out << "repeat=" << config_->repeat << "\n";
        out << "noSwap=" << (config_->noSwap ? "true" : "false") << "\n";
        out << "averageElo=" << config_->averageElo << "\n";
        out << "saveInterval=" << config_->saveInterval << "\n";

        out << "\n";
    }

    void TournamentData::loadTournamentConfig(const QaplaHelpers::IniFile::KeyValueMap keyValue) {
        for (const auto& [key, value] : keyValue) {
            if (key == "event") {
                config_->event = value;
            }
            else if (key == "type") {
                config_->type = value;
            }
            else if (key == "rounds") {
                config_->rounds = std::stoul(value);
            }
            else if (key == "games") {
                config_->games = std::stoul(value);
            }
            else if (key == "repeat") {
                config_->repeat = std::stoul(value);
            }
            else if (key == "noSwap") {
                config_->noSwap = (value == "true");
            }
            else if (key == "averageElo") {
                config_->averageElo = std::stoi(value);
            }
            else if (key == "saveInterval") {
                config_->saveInterval = std::stoul(value);
            }
        }
    }

    void TournamentData::saveOpeningConfig(std::ostream& out, const std::string& header) const {
		out << "[" << header << "]\n";
		out << "file=" << config_->openings.file << "\n";
		out << "format=" << config_->openings.format << "\n";
		out << "order=" << config_->openings.order << "\n";
        out << "seed=" << config_->openings.seed << "\n";
        if (config_->openings.plies) {
            out << "plies=" << *config_->openings.plies << "\n";
        }
        out << "start=" << config_->openings.start << "\n";
		out << "policy=" << config_->openings.policy << "\n";
		out << "\n";
	}

    void TournamentData::loadOpenings(const QaplaHelpers::IniFile::KeyValueMap keyValue) {
        for (auto [key, value] : keyValue) {
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
                config_->openings.seed = std::stoul(value);
            }
            else if (key == "plies") {
                config_->openings.plies = std::make_optional<int>(std::stoi(value));
            }
            else if (key == "start") {
                config_->openings.start = std::stoul(value);
            }
            else if (key == "policy" && (value == "default" || value == "encounter" || value == "round")) {
                config_->openings.policy = value;
            }
        }
    }

    void TournamentData::savePgnConfig(std::ostream& out, const std::string& header) const {
        out << "[" << header << "]\n";
        out << "file=" << pgnConfig_.file << "\n";
        out << "append=" << (pgnConfig_.append ? "true" : "false") << "\n";
        out << "onlyFinishedGames=" << (pgnConfig_.onlyFinishedGames ? "true" : "false") << "\n";
        out << "minimalTags=" << (pgnConfig_.minimalTags ? "true" : "false") << "\n";
        out << "saveAfterMove=" << (pgnConfig_.saveAfterMove ? "true" : "false") << "\n";
        out << "includeClock=" << (pgnConfig_.includeClock ? "true" : "false") << "\n";
        out << "includeEval=" << (pgnConfig_.includeEval ? "true" : "false") << "\n";
        out << "includePv=" << (pgnConfig_.includePv ? "true" : "false") << "\n";
        out << "includeDepth=" << (pgnConfig_.includeDepth ? "true" : "false") << "\n";
        out << "\n";
    }

    void TournamentData::loadPgnConfig(const QaplaHelpers::IniFile::KeyValueMap keyValue) {
        for (const auto& [key, value] : keyValue) {
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

        struct DrawAdjudicationConfig {
        uint32_t minFullMoves = 0;
        uint32_t requiredConsecutiveMoves = 0;
        int centipawnThreshold = 0;
        bool testOnly = false;
    };

    /**
     * @brief Configuration for resign adjudication logic.
     */
    struct ResignAdjudicationConfig {
        uint32_t requiredConsecutiveMoves = 0;
        int centipawnThreshold = 0;
        bool twoSided = false;
        bool testOnly = false;
    };

    void TournamentData::saveDrawAdjudicationConfig(std::ostream& out, const std::string& header) const {
        out << "[" << header << "]\n";
        out << "minFullMoves=" << drawConfig_.minFullMoves << "\n";
        out << "requiredConsecutiveMoves=" << drawConfig_.requiredConsecutiveMoves << "\n";
        out << "centipawnThreshold=" << drawConfig_.centipawnThreshold << "\n";
        out << "testOnly=" << (drawConfig_.testOnly ? "true" : "false") << "\n";
        out << "\n";
    }

    void TournamentData::loadDrawAdjudicationConfig(const QaplaHelpers::IniFile::KeyValueMap keyValue) {
        for (const auto& [key, value] : keyValue) {
            if (key == "minFullMoves") {
                drawConfig_.minFullMoves = std::stoul(value);
            }
            else if (key == "requiredConsecutiveMoves") {
                drawConfig_.requiredConsecutiveMoves = std::stoul(value);
            }
            else if (key == "centipawnThreshold") {
                drawConfig_.centipawnThreshold = std::stoi(value);
            }
            else if (key == "testOnly") {
                drawConfig_.testOnly = (value == "true");
            }
        }
    }

    void TournamentData::saveResignAdjudicationConfig(std::ostream& out, const std::string& header) const {
        out << "[" << header << "]\n";
        out << "requiredConsecutiveMoves=" << resignConfig_.requiredConsecutiveMoves << "\n";
        out << "centipawnThreshold=" << resignConfig_.centipawnThreshold << "\n";
        out << "twoSided=" << (resignConfig_.twoSided ? "true" : "false") << "\n";
        out << "testOnly=" << (resignConfig_.testOnly ? "true" : "false") << "\n";
        out << "\n";
    }

    void TournamentData::loadResignAdjudicationConfig(const QaplaHelpers::IniFile::KeyValueMap keyValue) {
        for (const auto& [key, value] : keyValue) {
            if (key == "requiredConsecutiveMoves") {
                resignConfig_.requiredConsecutiveMoves = std::stoul(value);
            }
            else if (key == "centipawnThreshold") {
                resignConfig_.centipawnThreshold = std::stoi(value);
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

    void TournamentData::saveTournamentResults(std::ostream& out, const std::string& header) const {
        if (tournament_) {
            tournament_->save(out, header);
        }
    }

    void TournamentData::loadConfig(QaplaHelpers::IniFile::SectionList sections) {
        for (const auto& section : sections) {
            const auto& sectionName = section.name;
            if (sectionName == "tournamentengine") {
                loadTournamentEngine(section.entries);
            } else if (sectionName == "tournamenteachengine") {
                loadEachEngineConfig(section.entries);
            } else if (sectionName == "tournament") {
                loadTournamentConfig(section.entries);
            } else if (sectionName == "tournamentopening") {
                loadOpenings(section.entries);
            } else if (sectionName == "tournamentpgn") {
                loadPgnConfig(section.entries);
            } else if (sectionName == "tournamentdrawadjudication") {
                loadDrawAdjudicationConfig(section.entries);
            } else if (sectionName == "tournamentresignadjudication") {
                loadResignAdjudicationConfig(section.entries);
            } 
        }
        if (createTournament(false)) {
            for (const auto& section : sections) {
                const auto& sectionName = section.name;
                if (sectionName == "tournamentround" && tournament_) {
                    tournament_->load(section);
                }
            }
        }
    }

}

