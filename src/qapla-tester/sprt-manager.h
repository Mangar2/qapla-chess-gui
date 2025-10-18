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

#include <tuple>
#include "engine-config.h"
#include "game-task.h"
#include "openings.h"
#include "pair-tournament.h"
#include "input-handler.h"
#include "ini-file.h"

/**
 * @brief Configuration parameters for a SPRT test run.
 */
struct SprtConfig {
    int eloUpper;
    int eloLower;
    double alpha;
    double beta;
    uint32_t maxGames;
    Openings openings;
};

/**
 * @brief Result of a SPRT computation containing all values for display.
 */
struct SprtResult {
    std::optional<bool> decision;  // true if H1 accepted, false if H0 accepted, nullopt if inconclusive
    double llr;                    // Log-Likelihood Ratio
    double lowerBound;             // Lower decision boundary
    double upperBound;             // Upper decision boundary
    double drawElo;                // Computed drawElo value
    int winsA;                     // Wins for engine A
    int draws;                     // Number of draws
    int winsB;                      // Wins for engine B
    std::string engineA;           // Name of engine A
    std::string engineB;           // Name of engine B
    int eloLower;                  // Lower elo bound from config
    int eloUpper;                  // Upper elo bound from config
};

 
/**
  * Manages the analysis of EPD test sets using multiple chess engines in parallel.
  * Provides GameTasks for engine workers and collects their results.
  */
class SprtManager : public GameTaskProvider {
public:
    SprtManager() = default;

    /**
     * @brief Initializes and starts the SPRT testing procedure between two engines.
     *
	 * @param engine0 Configuration for the first engine.
     * @param engine1 Configuration for the second engine.
     * @param config All configuration parameters required for the SPRT test.
     */
    void createTournament(const EngineConfig& engine0, const EngineConfig& engine1,
        const SprtConfig& config);

    /**
     * @brief Schedules the tournament and registers all pairings as task providers.
     *
     * @param self Shared pointer to this Tournament instance.
     * @param concurrency Number of parallel workers to use.
     */
    void schedule(const std::shared_ptr<SprtManager>& self, uint32_t concurrency);

    /**
     * @brief Provides the next available task.
     *
     * @return A GameTask with a unique taskId or std::nullopt if no task is available.
     */
    std::optional<GameTask> nextTask() override;

    /**
     * @brief Records the result of a finished game identified by taskId.
     *
     * @param taskId Identifier of the task this game result belongs to.
     * @param record Game outcome and metadata.
     */
    void setGameRecord(const std::string& taskId, const GameRecord& record) override;

    void runMonteCarloTest(const SprtConfig &config);
    void runMonteCarloSingleTest(const int simulationsPerElo, int elo, const double drawRate, int64_t &noDecisions, int64_t &numH0, int64_t &numH1, int64_t &totalGames);

    /**
	 * @brief Returns the current decision of the SPRT test.
	 * @return std::optional<bool> containing true if H1 accepted, false if H0 accepted, or std::nullopt if inconclusive.
	 */
	std::optional<bool> getDecision() const {
		return decision_;
	}

    /**
	 * @brief Saves the current SPRT test state to a stream.
	 * @param filename The file to save the state to.
     */
    void save(const std::string& filename) const;

    /**
     * @brief Loads the state from a stream - do nothing, if the file cannot be loaded.
	 * @param filename The file to load the state from.
     */
    void load(const QaplaHelpers::IniFile::Section& section);

    TournamentResult getResult() const {
        TournamentResult t;
		t.add(tournament_.getResult());
        return t;
    }

    /**
     * @brief Computes the result of the Sequential Probability Ratio Test (SPRT) using BayesElo model.
     *
     * Applies Jeffreys' prior, estimates drawElo, and compares likelihoods under H0 and H1.
     * Returns SprtResult containing decision, llr, bounds and all relevant values.
     */
    SprtResult computeSprt() const;

    /**
     * @brief Prints the SPRT result in a formatted way.
     * @param result The SPRT result to print.
     * @return A formatted string containing the SPRT decision or bounds.
     */
    static std::string printSprt(const SprtResult& result);

private:
    PairTournament tournament_;
    std::shared_ptr<StartPositions> startPositions_;

    /**
     * @brief Evaluates the current SPRT test state and logs result if decision boundary is reached.
     * @return true if the test should be stopped (H0 or H1 accepted), false otherwise.
     */
    
     /**
      * @brief Computes the result of the Sequential Probability Ratio Test (SPRT) using BayesElo model.
      *
      * Internal version with explicit parameters.
      */
    SprtResult computeSprt(
        int winsA, int draws, int winsB, const std::string& engineA, const std::string& engineB) const;
	bool rememberStop_ = false;

    SprtConfig config_;
	std::optional<bool> decision_ = std::nullopt;

    // Registration
    std::unique_ptr<InputHandler::CallbackRegistration> sprtCallback_;

};
