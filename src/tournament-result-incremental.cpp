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

#include "qapla-tester/tournament.h"
#include "qapla-tester/tournament-result.h"

using namespace QaplaWindows;

void TournamentResultIncremental::poll(const Tournament& tournament) {
	if (tournament.getUpdateCount() == tournamentUpdateCount_) {
		return;
	}
	constexpr size_t extraChecks = 10; // Number of extra results to fetch
	tournamentUpdateCount_ = tournament.getUpdateCount();

	// We store all results that are consecutively finished
	while (auto pairTournament = tournament.getPairTournament(currentIndex_)) {
		if (!(*pairTournament)->isFinished()) break;
		auto resultToAdd = (*pairTournament)->getResult();
		finishedResultsAggregate_.add(resultToAdd);
		engineNames_.insert(resultToAdd.getEngineA());
		engineNames_.insert(resultToAdd.getEngineB());
		updateCount_++;
		currentIndex_++;
	}

	// We add all results that are not consecutively finished including partial results
	totalResult_ = finishedResultsAggregate_;
	size_t extra = extraChecks;
	for (size_t i = currentIndex_; auto pairTournament = tournament.getPairTournament(i); ++i) {
		auto resultToAdd = (*pairTournament)->getResult();
		if (resultToAdd.total() == 0) {
			extra--;
			if (extra == 0) break; 
			continue;
		}
		totalResult_.add(resultToAdd);
		engineNames_.insert(resultToAdd.getEngineA());
		engineNames_.insert(resultToAdd.getEngineB());
	}
}

