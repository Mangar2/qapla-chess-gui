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

#include "qapla-tester/string-helper.h"

#include "qapla-tester/engine-option.h"
#include "qapla-tester/engine-worker-factory.h"

#include "qapla-tester/tournament.h"
#include "qapla-tester/tournament-result.h"

#include "qapla-tester/game-manager-pool.h"
#include "qapla-tester/pgn-io.h"
#include "qapla-tester/adjudication-manager.h"
#include "imgui-table.h"

#include "imgui.h"

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
		)
    { 
        runningTable_.setClickable(true);
        init();
    }

    TournamentData::~TournamentData() = default;

    void TournamentData:: init() {
    }

    void TournamentData::startTournament() {
        if (engineConfig_.empty()) {
            SnackbarManager::instance().showError("No engines configured for the tournament.");
            return;
		}
        std::vector<EngineConfig> selectedEngines;
        for (auto& tournamentConfig : engineConfig_) {
            if (!tournamentConfig.selected) continue;
            EngineConfig config = tournamentConfig.config;
            config.setPonder(eachEngineConfig_.ponder);
			config.setTimeControl(eachEngineConfig_.tc);
			config.setRestartOption(parseRestartOption(eachEngineConfig_.restart));
			config.setTraceLevel(eachEngineConfig_.traceLevel);
            config.setOptionValue("Hash", std::to_string(eachEngineConfig_.hash));
            selectedEngines.push_back(config);
		}

        if (!validateOpenings()) return;

        if (tournament_) {
            PgnIO::tournament().setOptions(pgnConfig_);
            AdjudicationManager::instance().setDrawAdjudicationConfig(drawConfig_);
            AdjudicationManager::instance().setResignAdjudicationConfig(resignConfig_);
            result_->setGamesLeft();
			tournament_->createTournament(selectedEngines, *config_);
            tournament_->scheduleAll(0, false);
            eloTable_.clear();
            populateEloTable();
			runningTable_.clear();
			populateRunningTable();
            imguiConcurrency_->init();
            imguiConcurrency_->setActive(true);
            running_ = true;
        } else {
            SnackbarManager::instance().showError("Internal error, tournament not initialized");
            return;
        }
		SnackbarManager::instance().showSuccess("Tournament started");
	}

    TournamentConfig& TournamentData::config() {
        return *config_;
	}

    void TournamentData::populateEloTable() {
        eloTable_.clear();

        struct Scored
        {
            std::string engineName; ///< The name of the engine
            EngineResult result;    ///< The duel result
            double score;           ///< Normalized score (0.0 to 1.0)
            double elo;             ///< Computed Elo rating
            double total;           ///< Total number of games played
            int error;              ///< Error margin for the Elo rating
        };
        for (auto scored : result_->getScoredEngines()) {
            std::vector<std::string> row;
            row.push_back(scored.engineName);
            row.push_back(std::format("{:.1f}", scored.elo));
            row.push_back("+/- " + std::to_string(scored.error));
            row.push_back(std::format("{:.1f}%", scored.score * 100.0));
            row.push_back(std::format("{:.0f}", scored.total));
            eloTable_.push(row);
		}
    }

    void TournamentData::populateRunningTable() {
        runningTable_.clear();
        if (tournament_) {
            for (auto& window: boardWindow_) {
                window.setRunning(false);
            }
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
                }
            },
            [&](uint32_t gameIndex) -> bool {
                return true;
            });
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
                populateEloTable();
            }
            populateRunningTable();
            populateViews();
        } 
    }

    bool TournamentData::isRunning() const {
        return running_ && result_->gamesLeft();
	}

    std::optional<size_t> TournamentData::drawEloTable(const ImVec2& size) const {
        return eloTable_.draw(size, true);
    }

    std::optional<size_t> TournamentData::drawRunningTable(const ImVec2& size) const {
        return runningTable_.draw(size, true);
	}

    void TournamentData::drawTabs() {
        uint32_t index = 0;
        for (auto& window : boardWindow_) {
            index++;
            if (!window.isRunning()) continue;
            std::string tabName = "Game " + window.id();
            if (ImGui::BeginTabItem((tabName + "###Game" + std::to_string(index)).c_str())) {
                if (window.isActive()) window.draw();
                window.setActive(true);
                ImGui::EndTabItem();
            } else {
                window.setActive(false);
            }
        }
    }

    void TournamentData::stopPool(bool graceful) {
        imguiConcurrency_->update(0);
        imguiConcurrency_->setActive(false);
        running_ = false;
        if (!graceful) GameManagerPool::getInstance().stopAll();
        SnackbarManager::instance().showSuccess(
            graceful ? 
                "Tournament stopped.\nFinishing ongoing games." : 
                "Tournament stopped"
        );
    }

    void TournamentData::clear() {
        imguiConcurrency_->setActive(false);
        running_ = false;
        GameManagerPool::getInstance().clearAll();
        tournament_ = std::make_unique<Tournament>();
        result_ = std::make_unique<TournamentResultIncremental>();
    }

    void TournamentData::setPoolConcurrency(uint32_t count, bool nice) {
        if (!isRunning()) return;
        imguiConcurrency_->setNiceStop(nice);
        imguiConcurrency_->update(count);
    }

    void TournamentData::saveTournamentEngines(std::ostream& out, const std::string& header) const {
        for (const auto& engine : engineConfig_) {
            out << "[" << header << "]\n";
            out << "name" << "=" << engine.config.getName() << "\n";
            out << "selected=" << (engine.selected ? "true" : "false") << "\n";
            out << "gauntlet=" << (engine.config.isGauntlet() ? "true" : "false") << "\n";
            out << "\n";
        }
    }

    void TournamentData::loadTournamentEngine(const QaplaConfiguration::ConfigMap& keyValue) {
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
        engineConfig_.push_back(engine);
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

    void TournamentData::loadEachEngineConfig(const QaplaConfiguration::ConfigMap& keyValue) {
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

    void TournamentData::loadTournamentConfig(const QaplaConfiguration::ConfigMap& keyValue) {
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

    void TournamentData::loadOpenings(const QaplaConfiguration::ConfigMap& keyValue) {
        for (auto [key, value] : keyValue) {
            if (key == "file") {
                config_->openings.file = value;
            }
            else if (key == "format") {
                config_->openings.format = value;
            }
            else if (key == "order") {
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
            else if (key == "policy") {
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

    void TournamentData::loadPgnConfig(const QaplaConfiguration::ConfigMap& keyValue) {
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

    void TournamentData::loadDrawAdjudicationConfig(const QaplaConfiguration::ConfigMap& keyValue) {
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

    void TournamentData::loadResignAdjudicationConfig(const QaplaConfiguration::ConfigMap& keyValue) {
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

}

