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
#include "game-context.h"
#include "engine-worker-factory.h"

GameContext::GameContext() = default;

void GameContext::initPlayers(std::vector<std::unique_ptr<EngineWorker>> engines) {
    players_.clear();
    for (auto& engine : engines) {
        if (eventCallback_) {
            engine->setEventSink(eventCallback_);
        }
        auto player = std::make_unique<PlayerContext>();
        player->setEngine(std::move(engine));
        players_.emplace_back(std::move(player));
    }
}

void GameContext::ensureStarted() {
    for (auto& player : players_) {
        if (player->getEngine()->isStopped()) {
            player->restartEngine(true);
            if (eventCallback_) {
                player->getEngine()->setEventSink(eventCallback_);
			}
        }
    }
}

void GameContext::restartPlayer(uint32_t index) {
    if (index >= players_.size()) return;
    players_[index]->restartEngine();
    if (eventCallback_) {
        players_[index]->getEngine()->setEventSink(eventCallback_);
    }
}

void GameContext::restartPlayer(const std::string& id) {
    for (auto& player : players_) {
        if (player->getIdentifier() == id) {
            player->restartEngine(true);
            if (eventCallback_) {
                player->getEngine()->setEventSink(eventCallback_);
            }
        }
    }
}

void GameContext::stopEngine(const std::string& id) {
    for (auto& player : players_) {
        if (player->getIdentifier() == id) {
            player->stopEngine();
        }
    }
}

void GameContext::setTimeControl(const TimeControl& timeControl) {
    for (auto& player : players_) {
        player->setTimeControl(timeControl);
    }
    std::lock_guard lock(gameRecordMutex_);
    gameRecord_.setTimeControl(timeControl, timeControl);
}

void GameContext::newGame() {
    ensureStarted();
    for (size_t i = 0; i < players_.size(); ++i) {
        bool isWhite = (i == 0 && !switchedSide_) || (i == 1 && switchedSide_);
        players_[i]->newGame(gameRecord_, isWhite);
    }
}

void GameContext::setPosition(bool useStartPosition, const std::string& fen,
    std::optional<std::vector<std::string>> playedMoves) {
    cancelCompute();
    auto* white = getWhite();
    auto* black = getBlack();
    const std::string whiteName = white && white->getEngine() ? white->getEngine()->getConfig().getName() : "";
    const std::string blackName = black && black->getEngine() ? black->getEngine()->getConfig().getName() : "";

    {
        std::lock_guard lock(gameRecordMutex_);
        gameRecord_.setStartPosition(useStartPosition, fen, true, whiteName, blackName);

        if (playedMoves) {
            for (const auto& move : *playedMoves) {
                MoveRecord moveRecord(gameRecord_.nextMoveIndex(), "#gui");
                moveRecord.original = move;
                moveRecord.lan = move;
                gameRecord_.addMove(moveRecord);
            }
        }
    }

    for (auto& player : players_) {
        player->setStartPosition(gameRecord_);
    }
}

void GameContext::setPosition(const GameRecord& record) {
    cancelCompute();
    {
        std::lock_guard lock(gameRecordMutex_);
        gameRecord_ = record;
        auto* white = getWhite();
        auto* black = getBlack();
        const std::string whiteName = white && white->getEngine() ? white->getEngine()->getConfig().getName() : "";
        const std::string blackName = black && black->getEngine() ? black->getEngine()->getConfig().getName() : "";

        gameRecord_.setWhiteEngineName(whiteName);
        gameRecord_.setBlackEngineName(blackName);
    }

    for (auto& player : players_) {
        player->setStartPosition(gameRecord_);
    }

}

size_t GameContext::getPlayerCount() const {
    return players_.size();
}

PlayerContext* GameContext::player(size_t index) {
    if (index >= players_.size()) return nullptr;
    return players_[index].get();
}

PlayerContext* GameContext::getWhite() {
    if (players_.empty()) return nullptr;
    return players_[(switchedSide_ ? 1 : 0) % players_.size()].get();
}

PlayerContext* GameContext::getBlack() {
    if (players_.size() < 2) return getWhite();
    return players_[(switchedSide_ ? 0 : 1) % players_.size()].get();
}

void GameContext::setSideSwitched(bool switched) {
    switchedSide_ = switched;
}

bool GameContext::isSideSwitched() const {
    return switchedSide_;
}

void GameContext::setEventCallback(std::function<void(EngineEvent&&)> callback) {
    eventCallback_ = std::move(callback);
    for (auto& player : players_) {
        if (player->getEngine()) {
            player->getEngine()->setEventSink(eventCallback_);
        }
    }
}

const GameRecord& GameContext::gameRecord() const {
    return gameRecord_;
}

void GameContext::withGameRecord(std::function<void(const GameRecord&)> accessFn) const {
    std::lock_guard lock(gameRecordMutex_);
    accessFn(gameRecord_);
}

std::tuple<GameEndCause, GameResult> GameContext::checkGameResult() {

    for (auto& player : players_) {
        auto [pcause, presult] = player->getGameResult();
        if (presult != GameResult::Unterminated) {
            std::lock_guard lock(gameRecordMutex_);
            gameRecord_.setGameEnd(pcause, presult);
            break;
        }
    }

    return gameRecord_.getGameResult();
}

bool GameContext::checkForTimeoutsAndRestart() {
    if (!eventCallback_) 
		throw AppError::make("GameContext::checkForTimeoutsAndRestart; No event callback set.");

    bool restarted = false;
    for (auto& player : players_) {
        if (player->checkEngineTimeout()) {
            restarted = true;
            player->getEngine()->setEventSink(eventCallback_);
        }
    }
    return restarted;
}

PlayerContext* GameContext::findPlayerByEngineId(const std::string& identifier) {
    for (auto& player : players_) {
        if (player->getEngine()->getIdentifier() == identifier) {
            return player.get();
        }
    }
    return nullptr;
}

void GameContext::restartIfConfigured() {
    for (auto& player : players_) {
        if (!player->getEngine()) continue;

        if (player->getEngine()->getConfig().getRestartOption() == RestartOption::Always) {
            player->restartEngine();
            if (eventCallback_) {
                player->getEngine()->setEventSink(eventCallback_);
			}
        }
    }
}

void GameContext::cancelCompute() {
    for (auto& player : players_) {
        player->cancelCompute();
    }
}

EngineRecords GameContext::getEngineRecords() const {
    EngineRecords records;
    for (const auto& player : players_) {
        if (!player->getEngine()) {
            EngineRecord record = {
                .identifier{},
                .config{},
                .supportedOptions{},
                .status = EngineRecord::Status::NotStarted,
                .memoryUsageB = 0,
                .curMoveRecord = player->getCurrentMove()
            };
            records.push_back(record);
            continue;
        }
		auto engine = player->getEngine();
        EngineRecord record = {
			.identifier = engine->getIdentifier(),
            .config = engine->getConfig(),
            .supportedOptions = engine->getSupportedOptions(),
            .memoryUsageB = engine->getEngineMemoryUsage(),
            .curMoveRecord = player->getCurrentMove()
        };
        switch (engine->workerState()) {
        case EngineWorker::WorkerState::notStarted: record.status = EngineRecord::Status::NotStarted; break;
        case EngineWorker::WorkerState::starting: record.status = EngineRecord::Status::Starting; break;
        case EngineWorker::WorkerState::running: record.status = EngineRecord::Status::Running; break;
        case EngineWorker::WorkerState::stopped: record.status = EngineRecord::Status::NotStarted; break;
        default: record.status = EngineRecord::Status::Error; break;
        }
        records.push_back(record);
    }
	return records;
}

