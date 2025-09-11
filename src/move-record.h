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

#include <string>
#include <optional>
#include <cassert>
#include <sstream>
#include <iomanip>
#include "engine-event.h"

struct MoveRecord {
    std::string original{};
    std::string lan{};
    std::string san{};
    std::string comment{};
    std::string nag{};
    uint64_t timeMs = 0;

    std::optional<int> scoreCp = std::nullopt;
    std::optional<int> scoreMate = std::nullopt;

    uint32_t halfmoveClock = 0; 
    uint32_t depth = 0;
    uint32_t seldepth = 0;
    uint32_t multipv = 1;
    uint64_t nodes = 0;
    std::string pv{};
	std::vector<SearchInfo> info; 
    uint32_t infoUpdateCount = 0;

    uint32_t halfmoveNo_ = 0;
    std::string engineId_{};

	MoveRecord() = default;
    MoveRecord(uint32_t halfmoveNo, const std::string& id = "")
		: halfmoveNo_(halfmoveNo), engineId_(id) {
	}

    void clear() {
		original.clear();
		lan.clear();
        san.clear();
		comment.clear();
		nag.clear();
		timeMs = 0;
		scoreCp.reset();
		scoreMate.reset();
		depth = 0;
		seldepth = 0;
		multipv = 1;
		nodes = 0;
		pv.clear();
        info.clear();
		halfmoveNo_ = 0;
		engineId_.clear();
    }

    /**
     * Updates the move record with the best move and time taken from an EngineEvent.
     *
     * @param event EngineEvent of type BestMove containing the chosen move and timestamp.
     * @param lanMove move in long algebraic notation
     * @param sanMove move in short algebraic notation
     * @param computeStartTimestamp Timestamp when move computation started, in milliseconds.
     * @param halfmoveClk The current halfmove clock value, used for the 50-move rule.
     */
    void updateFromBestMove(uint32_t halfmoveNo, const std::string& engineId, 
        const EngineEvent& event, std::string lanMove, std::string sanMove,
        uint64_t computeStartTimestamp, uint32_t halfmoveClk) {
		halfmoveNo_ = halfmoveNo;
		engineId_ = engineId;
        if (event.bestMove) {
            original = *event.bestMove;
			lan = lanMove;
            san = sanMove;
        }
        halfmoveClock = halfmoveClk;
        timeMs = event.timestampMs - computeStartTimestamp;
    }

    /**
	 * @brief Updates the move record with search information from an EngineEvent.
	 * @param info SearchInfo containing various search metrics.
     */
    void updateFromSearchInfo(const SearchInfo& info) {
        if (info.depth) depth = *info.depth;
        if (info.selDepth) seldepth = *info.selDepth;
        if (info.multipv) multipv = *info.multipv;
        if (info.nodes) nodes = static_cast<uint64_t>(*info.nodes);

        if (info.scoreCp) {
            scoreCp = *info.scoreCp;
            scoreMate.reset();
		}
		else if (info.scoreMate) {
			scoreMate = *info.scoreMate;
			scoreCp.reset();
		}

        if (!info.pv.empty()) {
            pv.clear();
            for (size_t i = 0; i < info.pv.size(); ++i) {
                if (i > 0) pv += ' ';
                pv += info.pv[i];
            }
        }
		infoUpdateCount++;
		// We keep the pv history, but everything else is overwritten
        if (!this->info.empty() && this->info.back().pv.empty()) {
            this->info.back().depth = depth;
            this->info.back().selDepth = seldepth;
            this->info.back().multipv = multipv;
			this->info.back().nodes = nodes;
			this->info.back().scoreCp = scoreCp;
			this->info.back().scoreMate = scoreMate;
            this->info.back().pv = info.pv;
            if (info.timeMs) {
                this->info.back().timeMs = info.timeMs;
            }
            if (info.hashFull) {
                this->info.back().hashFull = info.hashFull;
            }
            if (info.tbhits) {
                this->info.back().tbhits = info.tbhits;
            }
            if (info.cpuload) {
                this->info.back().cpuload = info.cpuload;
            }
            if (info.currMove) {
                this->info.back().currMove = info.currMove;
            }
            if (info.currMoveNumber) {
                this->info.back().currMoveNumber = info.currMoveNumber;
            }
            if (info.refutationIndex) {
                this->info.back().refutationIndex = info.refutationIndex;
            }
            if (!info.refutation.empty()) {
                this->info.back().refutation = info.refutation;
            }
        }
        else {
			this->info.push_back(info);
        }
    }

	/**
	 * @brief Returns a string representation of the score.
	 *
	 * @return A string representing the score in centipawns or mate value.
	 */
    std::string evalString() const {
		assert(!(scoreCp && scoreMate));
        if (scoreMate) {
            return *scoreMate > 0 ? "M" + std::to_string(*scoreMate) : "-M" + std::to_string(-*scoreMate);
        }
        if (scoreCp) {
            std::ostringstream oss;
            oss << (*scoreCp >= 0 ? "+" : "")
                << std::fixed << std::setprecision(2) << (*scoreCp / 100.0);
            return oss.str();
        }
        return "?";
    }
};
