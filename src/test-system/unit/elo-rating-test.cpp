/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include <catch2/catch_test_macros.hpp>

#include <game-manager/tournament-result.h>

#include <algorithm>
#include <cmath>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace QaplaTester;

namespace {

/**
 * @brief One aggregated duel: engineA vs engineB over all rounds played between them.
 */
struct DuelData {
    const char* engineA;
    const char* engineB;
    int winsA;
    int draws;
    int winsB;
};

/**
 * @brief A real 10-gauntlet tournament: 10 Qapla builds, each against the same 14
 * opponents, each pairing exactly 1000 games (10 rounds x 100).
 *
 * Captured from a real .qtour file rather than synthesised, because the effect under
 * test only shows up at this scale: the per-duel Elo estimates are fed back into the
 * opponents' ratings, so a hand-built two-engine case cannot reproduce it. Listed in
 * the same order the file stores them, since computeAllElos() updates ratings
 * sequentially and is therefore order-sensitive.
 */
const std::vector<DuelData> kDuels = {
    {"Qapla 0.3.2", "Viridithas 3.0.0", 417, 213, 370},
    {"Qapla 0.3.2", "Spike 1.4.1", 303, 305, 392},
    {"Qapla 0.3.2", "Amoeba 2.7.l64p-l", 272, 278, 450},
    {"Qapla 0.3.2", "Counter 3.5", 282, 276, 442},
    {"Qapla 0.3.2", "Drofa 3.1.0", 246, 285, 469},
    {"Qapla 0.3.2", "Leorik 2.5", 310, 273, 417},
    {"Qapla 0.3.2", "Lynx 1.6.0", 385, 237, 378},
    {"Qapla 0.3.2", "Marvin 3.4.0", 312, 271, 417},
    {"Qapla 0.3.2", "Polaris", 327, 268, 405},
    {"Qapla 0.3.2", "Reckless 0.4.0", 322, 226, 452},
    {"Qapla 0.3.2", "Simbelmyne 1.7.0", 384, 275, 341},
    {"Qapla 0.3.2", "Vajolet2 2.3", 253, 274, 473},
    {"Qapla 0.3.2", "Willow 2.9", 293, 270, 437},
    {"Qapla 0.3.2", "Winter 0.5", 408, 246, 346},
    {"Qapla 0.4.0 - 011", "Viridithas 3.0.0", 388, 214, 398},
    {"Qapla 0.4.0 - 011", "Spike 1.4.1", 286, 292, 422},
    {"Qapla 0.4.0 - 011", "Amoeba 2.7.l64p-l", 287, 246, 467},
    {"Qapla 0.4.0 - 011", "Counter 3.5", 296, 276, 428},
    {"Qapla 0.4.0 - 011", "Drofa 3.1.0", 222, 275, 503},
    {"Qapla 0.4.0 - 011", "Leorik 2.5", 336, 264, 400},
    {"Qapla 0.4.0 - 011", "Lynx 1.6.0", 370, 261, 369},
    {"Qapla 0.4.0 - 011", "Marvin 3.4.0", 328, 293, 379},
    {"Qapla 0.4.0 - 011", "Polaris", 313, 279, 408},
    {"Qapla 0.4.0 - 011", "Reckless 0.4.0", 332, 245, 423},
    {"Qapla 0.4.0 - 011", "Simbelmyne 1.7.0", 370, 275, 355},
    {"Qapla 0.4.0 - 011", "Vajolet2 2.3", 233, 269, 498},
    {"Qapla 0.4.0 - 011", "Willow 2.9", 269, 253, 478},
    {"Qapla 0.4.0 - 011", "Winter 0.5", 376, 268, 356},
    {"Qapla 0.4.0 - 012", "Viridithas 3.0.0", 393, 218, 389},
    {"Qapla 0.4.0 - 012", "Spike 1.4.1", 289, 275, 436},
    {"Qapla 0.4.0 - 012", "Amoeba 2.7.l64p-l", 280, 252, 468},
    {"Qapla 0.4.0 - 012", "Counter 3.5", 265, 278, 457},
    {"Qapla 0.4.0 - 012", "Drofa 3.1.0", 203, 267, 530},
    {"Qapla 0.4.0 - 012", "Leorik 2.5", 294, 273, 433},
    {"Qapla 0.4.0 - 012", "Lynx 1.6.0", 357, 244, 399},
    {"Qapla 0.4.0 - 012", "Marvin 3.4.0", 299, 272, 429},
    {"Qapla 0.4.0 - 012", "Polaris", 273, 256, 471},
    {"Qapla 0.4.0 - 012", "Reckless 0.4.0", 305, 252, 443},
    {"Qapla 0.4.0 - 012", "Simbelmyne 1.7.0", 320, 321, 359},
    {"Qapla 0.4.0 - 012", "Vajolet2 2.3", 224, 280, 496},
    {"Qapla 0.4.0 - 012", "Willow 2.9", 273, 252, 475},
    {"Qapla 0.4.0 - 012", "Winter 0.5", 360, 252, 388},
    {"Qapla 0.4.0 - 013", "Viridithas 3.0.0", 393, 230, 377},
    {"Qapla 0.4.0 - 013", "Spike 1.4.1", 295, 277, 428},
    {"Qapla 0.4.0 - 013", "Amoeba 2.7.l64p-l", 281, 274, 445},
    {"Qapla 0.4.0 - 013", "Counter 3.5", 276, 270, 454},
    {"Qapla 0.4.0 - 013", "Drofa 3.1.0", 231, 266, 503},
    {"Qapla 0.4.0 - 013", "Leorik 2.5", 326, 265, 409},
    {"Qapla 0.4.0 - 013", "Lynx 1.6.0", 383, 230, 387},
    {"Qapla 0.4.0 - 013", "Marvin 3.4.0", 321, 295, 384},
    {"Qapla 0.4.0 - 013", "Polaris", 315, 269, 416},
    {"Qapla 0.4.0 - 013", "Reckless 0.4.0", 315, 248, 437},
    {"Qapla 0.4.0 - 013", "Simbelmyne 1.7.0", 333, 290, 377},
    {"Qapla 0.4.0 - 013", "Vajolet2 2.3", 244, 272, 484},
    {"Qapla 0.4.0 - 013", "Willow 2.9", 286, 276, 438},
    {"Qapla 0.4.0 - 013", "Winter 0.5", 377, 263, 360},
    {"Qapla 0.4.0 - 017", "Viridithas 3.0.0", 420, 226, 354},
    {"Qapla 0.4.0 - 017", "Spike 1.4.1", 332, 318, 350},
    {"Qapla 0.4.0 - 017", "Amoeba 2.7.l64p-l", 328, 258, 414},
    {"Qapla 0.4.0 - 017", "Counter 3.5", 327, 263, 410},
    {"Qapla 0.4.0 - 017", "Drofa 3.1.0", 254, 289, 457},
    {"Qapla 0.4.0 - 017", "Leorik 2.5", 346, 278, 376},
    {"Qapla 0.4.0 - 017", "Lynx 1.6.0", 407, 256, 337},
    {"Qapla 0.4.0 - 017", "Marvin 3.4.0", 346, 279, 375},
    {"Qapla 0.4.0 - 017", "Polaris", 352, 260, 388},
    {"Qapla 0.4.0 - 017", "Reckless 0.4.0", 341, 255, 404},
    {"Qapla 0.4.0 - 017", "Simbelmyne 1.7.0", 387, 288, 325},
    {"Qapla 0.4.0 - 017", "Vajolet2 2.3", 261, 242, 497},
    {"Qapla 0.4.0 - 017", "Willow 2.9", 324, 277, 399},
    {"Qapla 0.4.0 - 017", "Winter 0.5", 424, 252, 324},
    {"Qapla 0.4.0 - 018", "Viridithas 3.0.0", 426, 226, 348},
    {"Qapla 0.4.0 - 018", "Spike 1.4.1", 367, 278, 355},
    {"Qapla 0.4.0 - 018", "Amoeba 2.7.l64p-l", 327, 266, 407},
    {"Qapla 0.4.0 - 018", "Counter 3.5", 292, 292, 416},
    {"Qapla 0.4.0 - 018", "Drofa 3.1.0", 249, 266, 485},
    {"Qapla 0.4.0 - 018", "Leorik 2.5", 347, 264, 389},
    {"Qapla 0.4.0 - 018", "Lynx 1.6.0", 386, 279, 335},
    {"Qapla 0.4.0 - 018", "Marvin 3.4.0", 330, 281, 389},
    {"Qapla 0.4.0 - 018", "Polaris", 329, 251, 420},
    {"Qapla 0.4.0 - 018", "Reckless 0.4.0", 341, 252, 407},
    {"Qapla 0.4.0 - 018", "Simbelmyne 1.7.0", 403, 259, 338},
    {"Qapla 0.4.0 - 018", "Vajolet2 2.3", 261, 264, 475},
    {"Qapla 0.4.0 - 018", "Willow 2.9", 290, 266, 444},
    {"Qapla 0.4.0 - 018", "Winter 0.5", 435, 272, 293},
    {"Qapla 0.4.0 - 020", "Viridithas 3.0.0", 436, 219, 345},
    {"Qapla 0.4.0 - 020", "Spike 1.4.1", 354, 306, 340},
    {"Qapla 0.4.0 - 020", "Amoeba 2.7.l64p-l", 341, 256, 403},
    {"Qapla 0.4.0 - 020", "Counter 3.5", 312, 299, 389},
    {"Qapla 0.4.0 - 020", "Drofa 3.1.0", 262, 269, 469},
    {"Qapla 0.4.0 - 020", "Leorik 2.5", 355, 271, 374},
    {"Qapla 0.4.0 - 020", "Lynx 1.6.0", 424, 254, 322},
    {"Qapla 0.4.0 - 020", "Marvin 3.4.0", 377, 293, 330},
    {"Qapla 0.4.0 - 020", "Polaris", 346, 268, 386},
    {"Qapla 0.4.0 - 020", "Reckless 0.4.0", 371, 253, 376},
    {"Qapla 0.4.0 - 020", "Simbelmyne 1.7.0", 406, 284, 310},
    {"Qapla 0.4.0 - 020", "Vajolet2 2.3", 262, 294, 444},
    {"Qapla 0.4.0 - 020", "Willow 2.9", 344, 216, 440},
    {"Qapla 0.4.0 - 020", "Winter 0.5", 381, 295, 324},
    {"Qapla 0.4.0 - 024", "Viridithas 3.0.0", 438, 227, 335},
    {"Qapla 0.4.0 - 024", "Spike 1.4.1", 336, 303, 361},
    {"Qapla 0.4.0 - 024", "Amoeba 2.7.l64p-l", 301, 294, 405},
    {"Qapla 0.4.0 - 024", "Counter 3.5", 321, 302, 377},
    {"Qapla 0.4.0 - 024", "Drofa 3.1.0", 285, 275, 440},
    {"Qapla 0.4.0 - 024", "Leorik 2.5", 364, 286, 350},
    {"Qapla 0.4.0 - 024", "Lynx 1.6.0", 476, 232, 292},
    {"Qapla 0.4.0 - 024", "Marvin 3.4.0", 347, 294, 359},
    {"Qapla 0.4.0 - 024", "Polaris", 374, 272, 354},
    {"Qapla 0.4.0 - 024", "Reckless 0.4.0", 372, 259, 369},
    {"Qapla 0.4.0 - 024", "Simbelmyne 1.7.0", 390, 295, 315},
    {"Qapla 0.4.0 - 024", "Vajolet2 2.3", 271, 277, 452},
    {"Qapla 0.4.0 - 024", "Willow 2.9", 336, 264, 400},
    {"Qapla 0.4.0 - 024", "Winter 0.5", 429, 245, 326},
    {"Qapla 0.4.0 - 025a", "Viridithas 3.0.0", 469, 209, 322},
    {"Qapla 0.4.0 - 025a", "Spike 1.4.1", 369, 291, 340},
    {"Qapla 0.4.0 - 025a", "Amoeba 2.7.l64p-l", 334, 278, 388},
    {"Qapla 0.4.0 - 025a", "Counter 3.5", 355, 280, 365},
    {"Qapla 0.4.0 - 025a", "Drofa 3.1.0", 322, 253, 425},
    {"Qapla 0.4.0 - 025a", "Leorik 2.5", 362, 262, 376},
    {"Qapla 0.4.0 - 025a", "Lynx 1.6.0", 439, 240, 321},
    {"Qapla 0.4.0 - 025a", "Marvin 3.4.0", 356, 287, 357},
    {"Qapla 0.4.0 - 025a", "Polaris", 384, 241, 375},
    {"Qapla 0.4.0 - 025a", "Reckless 0.4.0", 386, 241, 373},
    {"Qapla 0.4.0 - 025a", "Simbelmyne 1.7.0", 396, 292, 312},
    {"Qapla 0.4.0 - 025a", "Vajolet2 2.3", 317, 266, 417},
    {"Qapla 0.4.0 - 025a", "Willow 2.9", 345, 281, 374},
    {"Qapla 0.4.0 - 025a", "Winter 0.5", 474, 241, 285},
    {"Qapla 0.4.0 Pieces", "Viridithas 3.0.0", 432, 223, 345},
    {"Qapla 0.4.0 Pieces", "Spike 1.4.1", 388, 292, 320},
    {"Qapla 0.4.0 Pieces", "Amoeba 2.7.l64p-l", 320, 322, 358},
    {"Qapla 0.4.0 Pieces", "Counter 3.5", 344, 303, 353},
    {"Qapla 0.4.0 Pieces", "Drofa 3.1.0", 282, 279, 439},
    {"Qapla 0.4.0 Pieces", "Leorik 2.5", 364, 302, 334},
    {"Qapla 0.4.0 Pieces", "Lynx 1.6.0", 421, 281, 298},
    {"Qapla 0.4.0 Pieces", "Marvin 3.4.0", 365, 283, 352},
    {"Qapla 0.4.0 Pieces", "Polaris", 347, 287, 366},
    {"Qapla 0.4.0 Pieces", "Reckless 0.4.0", 387, 244, 369},
    {"Qapla 0.4.0 Pieces", "Simbelmyne 1.7.0", 412, 269, 319},
    {"Qapla 0.4.0 Pieces", "Vajolet2 2.3", 271, 307, 422},
    {"Qapla 0.4.0 Pieces", "Willow 2.9", 334, 269, 397},
    {"Qapla 0.4.0 Pieces", "Winter 0.5", 466, 264, 270},
};

/** @brief Builds the TournamentResult exactly as loading the tournament file would. */
TournamentResult buildResult() {
    TournamentResult result;
    for (const auto& data : kDuels) {
        EngineDuelResult duel(data.engineA, data.engineB);
        duel.winsEngineA = data.winsA;
        duel.draws = data.draws;
        duel.winsEngineB = data.winsB;
        result.add(duel);
    }
    return result;
}

/** @brief Points scored by an engine across all its duels (win = 1, draw = 0.5). */
double pointsOf(const TournamentResult& result, const std::string& name) {
    auto engineResult = result.forEngine(name);
    REQUIRE(engineResult.has_value());
    const auto aggregated = engineResult->aggregate(name);
    return aggregated.winsEngineA + 0.5 * aggregated.draws;
}

double eloOf(const std::vector<TournamentResult::Scored>& scored, const std::string& name) {
    const auto it = std::ranges::find_if(scored,
        [&name](const TournamentResult::Scored& entry) { return entry.engineName == name; });
    REQUIRE(it != scored.end());
    return it->elo;
}

const std::string kStronger = "Qapla 0.4.0 - 025a";
const std::string kWeaker = "Qapla 0.4.0 Pieces";

} // namespace

TEST_CASE("More points against an identical opponent field must not yield less Elo",
          "[elo]") {
    auto result = buildResult();

    // Precondition: both engines faced exactly the same opponents with the same number
    // of games, so their scores are directly comparable and the opponent field cancels
    // out -- only the points scored may decide the order.
    auto strongerResult = result.forEngine(kStronger);
    auto weakerResult = result.forEngine(kWeaker);
    REQUIRE(strongerResult.has_value());
    REQUIRE(weakerResult.has_value());

    auto opponentsOf = [](const EngineResult& engineResult) {
        std::vector<std::string> names;
        for (const auto& duel : engineResult.duels) {
            names.push_back(duel.getEngineB() + "/" + std::to_string(duel.total()));
        }
        std::ranges::sort(names);
        return names;
    };
    REQUIRE(opponentsOf(*strongerResult) == opponentsOf(*weakerResult));

    const double strongerPoints = pointsOf(result, kStronger);
    const double weakerPoints = pointsOf(result, kWeaker);
    REQUIRE(strongerPoints > weakerPoints);

    // The rating table uses these parameters (see TournamentResultView).
    const auto scored = result.computeAllElos(2600, 50, false);

    const double strongerElo = eloOf(scored, kStronger);
    const double weakerElo = eloOf(scored, kWeaker);

    INFO("points " << kStronger << " = " << strongerPoints
         << " vs " << kWeaker << " = " << weakerPoints
         << "  (difference " << strongerPoints - weakerPoints << ")");
    INFO("elo    " << kStronger << " = " << strongerElo
         << " vs " << kWeaker << " = " << weakerElo
         << "  (difference " << strongerElo - weakerElo << ")");

    CHECK(strongerElo > weakerElo);
}

TEST_CASE("All engines sharing one opponent field rank by points", "[elo]") {
    // Generalises the case above: every gauntlet engine here faced the same 14 opponents
    // over the same number of games, so points are the only thing that may order them.
    auto result = buildResult();
    const auto scored = result.computeAllElos(2600, 50, false);

    std::vector<std::pair<double, std::string>> byPoints;
    for (const auto& entry : scored) {
        if (entry.engineName.starts_with("Qapla")) {
            byPoints.emplace_back(pointsOf(result, entry.engineName), entry.engineName);
        }
    }
    REQUIRE(byPoints.size() == 10);
    std::ranges::sort(byPoints, std::greater{});

    for (std::size_t i = 0; i + 1 < byPoints.size(); ++i) {
        const auto& [morePoints, betterEngine] = byPoints[i];
        const auto& [fewerPoints, worseEngine] = byPoints[i + 1];
        INFO(betterEngine << " (" << morePoints << " points, elo "
             << eloOf(scored, betterEngine) << ") vs "
             << worseEngine << " (" << fewerPoints << " points, elo "
             << eloOf(scored, worseEngine) << ")");
        CHECK(eloOf(scored, betterEngine) > eloOf(scored, worseEngine));
    }
}

TEST_CASE("Ratings do not depend on the order duels are visited", "[elo]") {
    // The rating is fitted iteratively and engines are visited in sequence, so the fit
    // must converge to the same point regardless of that sequence -- otherwise results
    // shift with nothing but the insertion order of the underlying data.
    auto forward = buildResult();

    TournamentResult reversed;
    for (const auto& data : std::views::reverse(kDuels)) {
        EngineDuelResult duel(data.engineA, data.engineB);
        duel.winsEngineA = data.winsA;
        duel.draws = data.draws;
        duel.winsEngineB = data.winsB;
        reversed.add(duel);
    }

    const auto scoredForward = forward.computeAllElos(2600, 50, false);
    const auto scoredReversed = reversed.computeAllElos(2600, 50, false);

    REQUIRE(scoredForward.size() == scoredReversed.size());
    for (const auto& entry : scoredForward) {
        const double other = eloOf(scoredReversed, entry.engineName);
        INFO(entry.engineName << ": " << entry.elo << " vs " << other);
        CHECK(std::abs(entry.elo - other) < 0.01);
    }
}
