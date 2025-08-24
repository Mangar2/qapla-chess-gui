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

 // GameRecord.h
#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <string>
#include <map>
#include <iostream>

#include "move-record.h"
#include "time-control.h"
#include "game-result.h"

struct GameStruct {
	std::string fen;
	std::string lanMoves;
	std::string sanMoves;
	// For winboard the move the engine has sent in original format;
	std::string originalMove;
	bool isWhiteToMove;
};

/**
 * Stores a list of moves and manages current game state pointer.
 * Supports forward/backward navigation and time control evaluation.
 */
class GameRecord {
public:
	void setStartPosition(bool startPos, std::string startFen, bool isWhiteToMove);
	void setStartPosition(bool startPos, std::string startFen, bool isWhiteToMove,
		std::string whiteEngineName, std::string blackEngineName);


	/**
	 * @brief Initializes this GameRecord using another GameRecord (for PGN-based start setup).
	 * @param source Source GameRecord to extract setup information from.
	 * @param toPly Number of plies to import from the source game history.
	 * @param whiteEngineName White engine name to override.
	 * @param blackEngineName Black engine name to override.
	 */
	void setStartPosition(const GameRecord& source, uint32_t toPly,
		const std::string& whiteEngineName, const std::string& blackEngineName);

    /** Adds a move at the current ply position, overwriting any future moves. */
    void addMove(const MoveRecord& move);

    /** Returns the current ply index. */
    uint32_t nextMoveIndex() const;

    /** Sets the current ply (0 = before first move). */
    void setNextMoveIndex(uint32_t ply);

    /** Advances to the next ply if possible. */
    void advance();

    /** Rewinds to the previous ply if possible. */
    void rewind();

    /**
     * Returns the total time used by each side up to the current ply.
     *
     * @return A pair of milliseconds used: {whiteTime, blackTime}
     */
    std::pair<uint64_t, uint64_t> timeUsed() const;

    /** Returns const reference to move history. */
	const std::vector<MoveRecord>& history() const {
		return moves_;
	}
	std::vector<MoveRecord>& history() {
		return moves_;
	}

    /**
	 * @brief returns true if the game started with the standard starting position.
     */
	bool getStartPos() const { return startPos_; }

	/**
	 * @brief Returns the starting position in FEN format.
	 * @return The starting position as a FEN string.
	 */
	std::string getStartFen() const { return startFen_; }

    /** 
	 * @brief Sets the game end cause and result.
	 * @param cause The cause of the game end.
	 * @param result The result of the game.
     */
	void setGameEnd(GameEndCause cause, GameResult result) {
		updateCnt_++;
		gameEndCause_ = cause;
		gameResult_ = result;
	}
	/**
	 * @brief Returns the game end cause and result.
	 * @return A pair of GameEndCause and GameResult.
	 */
	std::tuple<GameEndCause, GameResult> getGameResult() const {
		return { gameEndCause_, gameResult_ };
	}
    /**
	 * @brief Sets the time control for the game.
     */
    void setTimeControl(const TimeControl& whiteTimeControl, const TimeControl& blackTimeControl) {
		updateCnt_++;
        whiteTimeControl_ = whiteTimeControl;
		blackTimeControl_ = blackTimeControl;
    }
    /**
     * @brief Returns the white side's time control.
     */
    const TimeControl& getWhiteTimeControl() const {
        return whiteTimeControl_;
    }
	TimeControl& getWhiteTimeControl() {
		return whiteTimeControl_;
	}

    /**
     * @brief Returns the black side's time control.
     */
    const TimeControl& getBlackTimeControl() const {
        return blackTimeControl_;
    }
	TimeControl& getBlackTimeControl() {
		return blackTimeControl_;
	}

	/**
	 * @brief Returns the current side to move.
	 */
	bool isWhiteToMove() const {
		return wtmAtPly(currentPly_);
	}

	/**
	 * @brief Determines who was to move at the start of the game.
	 * @param game The game record.
	 * @return True if white was to move at ply 0, false if black.
	 */
	bool wtmAtPly(size_t ply) const {
		return ply % 2 == 0 ? isWhiteToMoveAtStart_ : !isWhiteToMoveAtStart_;
	}

	const std::string& getWhiteEngineName() const {
		return whiteEngineName_;
	}
	void setWhiteEngineName(const std::string& name) {
		updateCnt_++;
		whiteEngineName_ = name;
	}

    const std::string& getBlackEngineName() const {
		return blackEngineName_;
    }
	void setBlackEngineName(const std::string& name) {
		updateCnt_++;
		blackEngineName_ = name;
	}

	/**
	 * @brief Returns the round number of the game.
	 */
	uint32_t getRound() const {
		return round_;
	}
	/**
	 * @brief Sets the round number of the game.
	 * @param r The round number to set.
	 */
	void setRound(uint32_t r) {
		updateCnt_++;
		round_ = r;
	}

	/**
	 * @brief Sets a PGN tag key-value pair.
	 * @param key The tag name.
	 * @param value The tag value.
	 */
	void setTag(const std::string& key, const std::string& value) {
		updateCnt_++;
		if (value.empty()) {
			tags_.erase(key);
		}
		else {
			tags_[key] = value;
		}
	}

	/**
	 * @brief Gets the value of a PGN tag by key.
	 * @param key The tag name.
	 * @return Tag value or empty string if not found.
	 */
	std::string getTag(const std::string& key) const {
		auto it = tags_.find(key);
		return it != tags_.end() ? it->second : "";
	}
	/**
	 * @brief Gets all stored PGN tags.
	 * @return Map of all tag key-value pairs.
	 */
	const std::map<std::string, std::string>& getTags() const {
		return tags_;
	}

	/**
	 * @brief Checks if this game record is an update from another.
	 * @param other The other game record to compare with.
	 * @return True if the records differ, false if they are the same.
	 */
	bool isUpdate(const GameRecord& other) const;

	/**
	 * @brief Checks if this game record is different from another.
	 * @param other The other game record to compare with.
	 * @return True if the records differ, false if they are the same.
	 */
	bool isDifferent(const GameRecord& other) const;

	/**
	 * @brief Creates a GameStruct containing the essential game data.
	 *
	 * This method generates a `GameStruct` object that includes the starting FEN
	 * position and a concatenated list of moves in both LAN (long algebraic notation)
	 * and SAN (short algebraic notation). 
	 *
	 * @return A `GameStruct` object 
	 */
	GameStruct createGameStruct() const;

	/**
	 * @brief Reserves memory for the move history to avoid repeated reallocations.
	 *
	 * This method pre-allocates memory for the internal move history (`moves_`),
	 * ensuring that no additional memory allocations are required when adding
	 * the specified number of moves. This is useful when the number of moves
	 * to be added is known in advance, improving performance.
	 *
	 * @param count The number of moves to reserve space for.
	 */
	void reserveMoves(size_t count) {
		moves_.reserve(count);
	}

	GameRecord createMinimalCopy() const;
	
private:
	
	std::map<std::string, std::string> tags_;
    bool startPos_ = true;
    std::string startFen_;
    std::vector<MoveRecord> moves_;
    uint32_t currentPly_ = 0;
    GameEndCause gameEndCause_ = GameEndCause::Ongoing;
	GameResult gameResult_ = GameResult::Unterminated;
    TimeControl whiteTimeControl_;
    TimeControl blackTimeControl_;
    std::string whiteEngineName_;
    std::string blackEngineName_;
	bool isWhiteToMoveAtStart_ = true; 
    uint32_t round_ = 0;
	uint64_t updateCnt_ = 1; 
};
