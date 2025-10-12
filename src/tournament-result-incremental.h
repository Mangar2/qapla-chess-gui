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

#include "qapla-tester/tournament-result.h"

#include <unordered_set>

class Tournament;

namespace QaplaWindows {


	class TournamentResultIncremental {
    public: 
        bool poll(const Tournament& tournament, double baseElo);

        /*
		 * @brief clears the internal state, removing all results and resetting counters.
         */
        void clear() {
            finishedResultsAggregate_.clear();
			totalResult_.clear();
            updateCount_ = 0;
            currentIndex_ = 0;
            totalScheduledGames_ = 0;
            playedGamesFromCompletedPairs_ = 0;
            playedGamesFromPartialPairs_ = 0;
		}

		/**
		 * @brief Returns the update count of the tournament results.
		 * @return The total number of updates for change detection.
         */
        uint64_t getUpdateCount() const {
            return updateCount_;
		}
		
        /**
		 * @brief Returns the total aggregated result of the tournament. 
         * 
		 * It includes both finished and non-finished tournament pairings.
         */ 
        const TournamentResult& getResult() const {
            return totalResult_;
		}

        /**
		 * @brief Returns the scored engines with their current results.
         * 
		 * @return Vector of scored engines with their names, results, and normalized scores.
		 */
        const std::vector<TournamentResult::Scored>& getScoredEngines() const {
            return totalResult_.scoredEngines();
        }

        /**
         * @brief True, if the tournament has games left to play
         */
        bool hasGamesLeft() const {
            return gamesLeft_;
		}

        /**
		 * @brief Sets the gamesLeft flag to true, indicating that there are now games to be played.
         * 
		 */
        void setGamesLeft() {
            gamesLeft_ = true;
        }

        /**
         * @brief Returns the total number of games scheduled in the tournament.
         * @return Total number of games that will be played.
         */
        uint32_t getTotalScheduledGames() const {
            return totalScheduledGames_;
        }

        /**
         * @brief Returns the number of games that have been completed.
         * @return Number of finished games.
         */
        uint32_t getPlayedGames() const {
            return playedGamesFromCompletedPairs_ + playedGamesFromPartialPairs_;
        }
			

    private:

        TournamentResult finishedResultsAggregate_;
		TournamentResult totalResult_; ///< total sum of finalized and non finalized results
        std::unordered_set<std::string> engineNames_;

		uint64_t updateCount_ = 0; ///< Number of updates 
		size_t currentIndex_ = 0; 
		size_t tournamentUpdateCount_ = 0; ///< Tournament update count, used to track changes
		bool gamesLeft_ = false; ///< Flag to indicate if the tournament is currently running
        
        uint32_t totalScheduledGames_ = 0; ///< Total number of games scheduled in the tournament
        uint32_t playedGamesFromCompletedPairs_ = 0; ///< Number of games that have been completed
        uint32_t playedGamesFromPartialPairs_ = 0; ///< Number of games that have been completed in pairs not fully completed
    };

}
