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

#include "tournament-result.h"
#include "change-tracker.h"

#include <unordered_set>

namespace QaplaTester {
    class Tournament;
}

namespace QaplaWindows {


	class TournamentResultIncremental {
    public: 
        bool poll(const QaplaTester::Tournament& tournament, double baseElo);

        /**
		 * @brief Returns the total aggregated result of the tournament. 
         * 
		 * It includes both finished and non-finished tournament pairings.
         */ 
        const QaplaTester::TournamentResult& getResult() const {
            return totalResult_;
		}

        /**
		 * @brief Returns the scored engines with their current results.
         * 
		 * @return Vector of scored engines with their names, results, and normalized scores.
		 */
        const std::vector<QaplaTester::TournamentResult::Scored>& getScoredEngines() const {
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
        /**
         * @brief Clears the internal state, removing all results and resetting counters.
         * @note This is private as it's only called internally by handleModification().
         * External callers should rely on poll() to detect modifications automatically.
         */
        void clear();

        /**
         * @brief Adds a finished pair tournament to the finished results aggregate.
         * @param pairIndex Index of the pair tournament
         * @param tournament Tournament containing the pair tournament
         */
        void addFinishedPairTournament(size_t pairIndex, const QaplaTester::Tournament& tournament);

        /**
         * @brief Handles tournament modification by rebuilding all state.
         * @param tournament Tournament to process
         * @param baseElo Base Elo rating for calculations
         */
        void handleModification(const QaplaTester::Tournament& tournament, double baseElo);

        QaplaTester::TournamentResult finishedResultsAggregate_;
		QaplaTester::TournamentResult totalResult_; ///< total sum of finalized and non finalized results
        std::unordered_set<std::string> engineNames_;
        std::vector<size_t> notFinishedIndices_; ///< Indices of pair tournaments that are not finished

		size_t currentIndex_ = 0; ///< Index into notFinishedIndices_ (not direct pair tournament index)
		QaplaTester::ChangeTracker changeTracker_;
		bool gamesLeft_ = false; ///< Flag to indicate if the tournament is currently running
        
        uint32_t totalScheduledGames_ = 0; ///< Total number of games scheduled in the tournament
        uint32_t pairTournaments_ = 0; ///< Number of pair tournaments in the overall tournament
        uint32_t playedGamesFromCompletedPairs_ = 0; ///< Number of games that have been completed
        uint32_t playedGamesFromPartialPairs_ = 0; ///< Number of games that have been completed in pairs not fully completed
    };

}
