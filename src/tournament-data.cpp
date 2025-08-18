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
#include "snackbar.h"

#include "qapla-tester/string-helper.h"

#include "qapla-tester/engine-option.h"
#include "qapla-tester/engine-worker-factory.h"

#include "qapla-tester/tournament.h"
#include "qapla-tester/tournament-result.h"

#include "qapla-tester/game-manager-pool.h"
#include "imgui-table.h"

#include "imgui.h"

namespace QaplaWindows {

    TournamentData::TournamentData() : 
        tournament_(std::make_unique<Tournament>()),
		config_(std::make_unique<TournamentConfig>()),
        result_(std::make_unique<TournamentResult>()),
        table_(
            "TournamentResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { "Name", ImGuiTableColumnFlags_WidthFixed, 150.0f },
                { "Ratio", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Wins", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Draws", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
                { "Losses", ImGuiTableColumnFlags_WidthFixed, 50.0f, true }
            }
        )
    { 
        table_.setClickable(true);
        init();
    }

    TournamentData::~TournamentData() = default;

    void TournamentData:: init() {
        auto c1 = EngineWorkerFactory::getConfigManager().getConfig("Qapla 0.4.0");
		auto c2 = EngineWorkerFactory::getConfigManager().getConfig("Spike 1.4.1");
		if (c1) engineConfig_.push_back(*c1);
        if (c2) engineConfig_.push_back(*c2);
    }

    void TournamentData::startTournament() {
		engineConfig_.clear();
        auto c1 = EngineWorkerFactory::getConfigManager().getConfig("Qapla 0.4.0");
        auto c2 = EngineWorkerFactory::getConfigManager().getConfig("Spike 1.4.1");
        if (c1) engineConfig_.push_back(*c1);
        if (c2) engineConfig_.push_back(*c2);
        if (engineConfig_.empty()) {
            SnackbarManager::instance().showError("No engines configured for the tournament.");
            return;
		}
		engineConfig_[0].setGauntlet(true);
        for (auto& engine : engineConfig_) {
            engine.setPonder(eachEngineConfig_.ponder);
			engine.setTimeControl(eachEngineConfig_.tc);
			engine.setRestartOption(parseRestartOption(eachEngineConfig_.restart));
			engine.setTraceLevel(eachEngineConfig_.traceLevel);
            engine.setOptionValue("Hash", std::to_string(eachEngineConfig_.hash));
		}

        if (!validateOpenings()) return;

        if (tournament_) {
			tournament_->createTournament(engineConfig_, *config_);
            tournament_->scheduleAll(concurrency_, false);
            table_.clear();
            populateTable();
        } else {
            SnackbarManager::instance().showError("Internal error, tournamen not initialized");
            return;
        }
		SnackbarManager::instance().showSuccess("Tournament started");
	}

    TournamentConfig& TournamentData::config() {
        return *config_;
	}

    void TournamentData::populateTable() {
        table_.clear();
        auto summary = result_->getSummary();
        for (auto& row : result_->getSummary()) {
            table_.push(row);
        }
    }

    void TournamentData::pollData() {
        if (tournament_) {
            auto [cnt, result] = tournament_->pollResult(updateCnt_);
			updateCnt_ = cnt;
            if (result) {
                *result_ = std::move(*result);
                populateTable();
            }
        } 
    }

    void TournamentData::clear() {
	}

    std::optional<size_t> TournamentData::drawTable(const ImVec2& size) const {
        return table_.draw(size);
    }

    void TournamentData::stopPool() {
        GameManagerPool::getInstance().stopAll();
        SnackbarManager::instance().showSuccess("Tournament stopped");
    }

    void TournamentData::clearPool() {
        GameManagerPool::getInstance().clearAll();
        tournament_ = std::make_unique<Tournament>();
    }

    void TournamentData::setPoolConcurrency(uint32_t count, bool nice, bool start) {
        GameManagerPool::getInstance().setConcurrency(count, nice, start);
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



    bool TournamentData::validateOpenings() {
        bool valid = true;
        if (config_->openings.file.empty()) {
            SnackbarManager::instance().showError("No openings file specified.");
            valid = false;
        }
        return valid;
	}

}

