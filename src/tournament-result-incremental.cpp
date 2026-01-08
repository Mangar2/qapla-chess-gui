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

#include "tournament-result-incremental.h"

#include "tournament/tournament.h"
#include "game-manager/tournament-result.h"

using QaplaTester::Tournament;
using namespace QaplaWindows;

void TournamentResultIncremental::addFinishedPairTournament(size_t pairIndex, const Tournament& tournament) {
	auto pairTournament = tournament.getPairTournament(pairIndex);
	if (!pairTournament || !(*pairTournament)->isFinished()) {
		return;
	}
	auto resultToAdd = (*pairTournament)->getResult();
	playedGamesFromCompletedPairs_ += static_cast<uint32_t>(resultToAdd.total());
	finishedResultsAggregate_.add(resultToAdd);
	engineNames_.insert(resultToAdd.getEngineA());
	engineNames_.insert(resultToAdd.getEngineB());
}

void TournamentResultIncremental::handleModification(const Tournament& tournament, double baseElo) {
	clear();
	
	pairTournaments_ = tournament.pairTournamentCount();
	
	// Calculate total scheduled games
	totalScheduledGames_ = 0;
	for (size_t i = 0; auto pairTournament = tournament.getPairTournament(i); ++i) {
		totalScheduledGames_ += (*pairTournament)->getConfig().games;
	}
	
	// Add all finished pair tournaments and collect unfinished indices
	for (size_t i = 0; auto pairTournament = tournament.getPairTournament(i); ++i) {
		if ((*pairTournament)->isFinished()) {
			addFinishedPairTournament(i, tournament);
		} else {
			notFinishedIndices_.push_back(i);
		}
	}
	
	// Add all partial results from unfinished pair tournaments
	totalResult_ = finishedResultsAggregate_;
	playedGamesFromPartialPairs_ = 0;
	for (auto idx : notFinishedIndices_) {
		auto pairTournament = tournament.getPairTournament(idx);
		if (!pairTournament) {
			continue;
		}
		auto resultToAdd = (*pairTournament)->getResult();
		playedGamesFromPartialPairs_ += static_cast<uint32_t>(resultToAdd.total());
		if (resultToAdd.total() > 0) {
			totalResult_.add(resultToAdd);
			engineNames_.insert(resultToAdd.getEngineA());
			engineNames_.insert(resultToAdd.getEngineB());
		}
	}
	
	gamesLeft_ = !notFinishedIndices_.empty();
	currentIndex_ = 0;
	totalResult_.computeAllElos(baseElo, 10, true);
}

bool TournamentResultIncremental::poll(const Tournament& tournament, double baseElo) {
	auto [isModified, isUpdated] = changeTracker_.checkModification(tournament.getChangeTracker());
	
	if (!isUpdated) {
		return false;
	}
	
	changeTracker_.updateFrom(tournament.getChangeTracker());
	
	if (isModified) {
		handleModification(tournament, baseElo);
		return true;
	}
	
	// Update case: check for newly finished tournaments
	gamesLeft_ = false;
	constexpr size_t extraChecks = 10; // Number of extra results to fetch
	
	// Process notFinishedIndices_ list to find newly finished tournaments
	while (currentIndex_ < notFinishedIndices_.size()) {
		size_t pairIndex = notFinishedIndices_[currentIndex_];
		auto pairTournament = tournament.getPairTournament(pairIndex);
		
		if (!pairTournament || !(*pairTournament)->isFinished()) {
			break;
		}
		
		addFinishedPairTournament(pairIndex, tournament);
		currentIndex_++;
	}
	
	// Rebuild totalResult_ with all results (finished + partial)
	totalResult_ = finishedResultsAggregate_;
	playedGamesFromPartialPairs_ = 0;
	size_t extra = extraChecks;
	
	for (size_t i = currentIndex_; i < notFinishedIndices_.size(); ++i) {
		size_t pairIndex = notFinishedIndices_[i];
		auto pairTournament = tournament.getPairTournament(pairIndex);
		
		if (!pairTournament) {
			continue;
		}
		
		auto resultToAdd = (*pairTournament)->getResult();
		playedGamesFromPartialPairs_ += static_cast<uint32_t>(resultToAdd.total());
		gamesLeft_ = true;
		
		if (resultToAdd.total() == 0) {
			extra--;
			if (extra == 0) {
				break;
			}
			continue;
		}
		
		totalResult_.add(resultToAdd);
		engineNames_.insert(resultToAdd.getEngineA());
		engineNames_.insert(resultToAdd.getEngineB());
	}
	
	totalResult_.computeAllElos(baseElo, 10, true);
	return true;
}


 void TournamentResultIncremental::clear() {
	finishedResultsAggregate_.clear();
	totalResult_.clear();
	engineNames_.clear();
	notFinishedIndices_.clear();
	currentIndex_ = 0;
	totalScheduledGames_ = 0;
	playedGamesFromCompletedPairs_ = 0;
	playedGamesFromPartialPairs_ = 0;
}
