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

#pragma once

#include "qapla-tester/engine-config.h"
#include "qapla-tester/pgn-io.h"
#include "qapla-tester/adjucation-manager.h"
#include "imgui-table.h"
#include <memory>
#include <optional>

class Tournament;
struct TournamentConfig;

namespace QaplaWindows {


	class TournamentData {
    public: 
        TournamentData();
        ~TournamentData();

        void init();

        void startTournament();

        /**
		 * @brief Polls the EPD data for new entries.
		 */
        void pollData();

        /**
		 * @brief Clears the current analysis results.
		 */
        void clear();

        /**
         * @brief Stops all ongoing tasks in the pool.
         */
        void stopPool();

        /**
         * @brief Stops all ongoing tasks and clears all task providers in the pool.
         */
        void clearPool();

        /**
         * @brief Sets the pool concurrency level.
         * @param count The number of concurrent tasks to allow.
         * @param nice If true, reduces the number of active managers gradually.
         * @param start If true, starts new tasks immediately.
         */
        void setPoolConcurrency(uint32_t count, bool nice = true, bool start = false);


        /**
         * @brief Draws the EPD test results table.
         * @param size Size of the table to draw.
         * @return The index of the selected row, or std::nullopt if no row was selected.
		 */
        std::optional<size_t> drawTable(const ImVec2& size) const;
        
        TournamentConfig& config();

        const std::vector<EngineConfig>& getEngineConfigs() const {
            return engineConfig_;
		}
        std::vector<EngineConfig>& getEngineConfigs() {
            return engineConfig_;
        }
        uint32_t& concurrency() {
            return concurrency_;
		}

        const PgnIO::Options& pgnConfig() const {
            return pgnConfig_;
		}

        PgnIO::Options& pgnConfig() {
            return pgnConfig_;
        }

        const AdjudicationManager::DrawAdjudicationConfig& drawConfig() const {
            return drawConfig_;
        }
        AdjudicationManager::DrawAdjudicationConfig& drawConfig() {
            return drawConfig_;
        }
		const AdjudicationManager::ResignAdjudicationConfig& resignConfig() const {
            return resignConfig_;
        }
        AdjudicationManager::ResignAdjudicationConfig& resignConfig() {
            return resignConfig_;
        }

	private:
        void validateOpenings();

        uint64_t updateCnt = 0;
        void populateTable();

		std::unique_ptr<Tournament> tournament_;
        std::unique_ptr<TournamentConfig> config_;
		PgnIO::Options pgnConfig_;
		AdjudicationManager::DrawAdjudicationConfig drawConfig_;
		AdjudicationManager::ResignAdjudicationConfig resignConfig_;
        std::vector<EngineConfig> engineConfig_{};
        uint32_t concurrency_ = 1;

        ImGuiTable table_;

    };

}
