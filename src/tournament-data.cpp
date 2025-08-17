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
#include "qapla-tester/engine-worker-factory.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/tournament.h"
#include "qapla-tester/game-manager-pool.h"
#include "imgui-table.h"

#include "imgui.h"

namespace QaplaWindows {

    TournamentData::TournamentData() : 
        tournament_(std::make_unique<Tournament>()),
		config_(std::make_unique<TournamentConfig>()),
        table_(
            "TournamentResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { "Name", ImGuiTableColumnFlags_WidthFixed, 160.0f },
                { "Best move", ImGuiTableColumnFlags_WidthFixed, 100.0f },
                { "Result", ImGuiTableColumnFlags_WidthFixed, 100.0f, true }
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

        if (tournament_) {
			tournament_->createTournament(engineConfig_, *config_);
            tournament_->scheduleAll(concurrency_);
            table_.clear();
            populateTable();
        } else {
            SnackbarManager::instance().showError("Internal error, tournamen not initialized");
        }
	}

    TournamentConfig& TournamentData::config() {
        return *config_;
	}

    void TournamentData::populateTable() {
    }

    void TournamentData::pollData() {
    }

    void TournamentData::clear() {
	}

    std::optional<size_t> TournamentData::drawTable(const ImVec2& size) const {
        return table_.draw(size);
    }

    void TournamentData::stopPool() {
        GameManagerPool::getInstance().stopAll();
    }

    void TournamentData::clearPool() {
        GameManagerPool::getInstance().clearAll();
    }

    void TournamentData::setPoolConcurrency(uint32_t count, bool nice, bool start) {
        GameManagerPool::getInstance().setConcurrency(count, nice, start);
    }

    void TournamentData::validateOpenings() {
        if (config_->openings.file.empty()) {
            throw AppError::makeInvalidParameters("Openings file is not set.");
        }
        if (config_->openings.format != "epd" && config_->openings.format != "raw" && config_->openings.format != "pgn") {
            throw AppError::makeInvalidParameters("Unsupported openings format: " + config_->openings.format);
        }
        if (config_->openings.order != "random" && config_->openings.order != "sequential") {
            throw AppError::makeInvalidParameters("Unsupported openings order: " + config_->openings.order);
        }
	}

}

