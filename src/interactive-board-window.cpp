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

#include "interactive-board-window.h"
#include "configuration.h"

#include "qapla-engine/move.h"
#include "qapla-tester/time-control.h"
#include "qapla-tester/game-state.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/compute-task.h"
#include "qapla-tester/engine-config.h"
#include "qapla-tester/engine-config-manager.h"
#include "qapla-tester/engine-worker-factory.h"
#include "qapla-tester/game-manager-pool.h"

#include "imgui-board.h"
#include "imgui-engine-list.h"
#include "imgui-clock.h"
#include "imgui-move-list.h"
#include "imgui-barchart.h"

using namespace QaplaWindows;

InteractiveBoardWindow::InteractiveBoardWindow()
	: gameRecord_(std::make_unique<GameRecord>()),
	  computeTask_(std::make_unique<ComputeTask>()),
	  imGuiBoard_(std::make_unique<ImGuiBoard>()),
	  imGuiEngineList_(std::make_unique<ImGuiEngineList>()),
	  imGuiClock_(std::make_unique<ImGuiClock>()),
	  imGuiMoveList_(std::make_unique<ImGuiMoveList>()),
	  imGuiBarChart_(std::make_unique<ImGuiBarChart>())
{
	timeControl_ = QaplaConfiguration::Configuration::instance()
					   .getTimeControlSettings()
					   .getSelectedTimeControl();
	computeTask_->setTimeControl(timeControl_);
	computeTask_->setPosition(true);
	epdData_.init();
}

InteractiveBoardWindow::~InteractiveBoardWindow() = default;

void InteractiveBoardWindow::saveConfig(std::ostream &out) const
{
	uint32_t index = 0;
	for (auto &engine: engineConfigs_) {
		out << "[boardengine]\n";
		out << "name=" << engine.getName() << '\n';
		out << "index=" << index << '\n';
		out << '\n';
		++index;
	}
}

void InteractiveBoardWindow::loadBoardEngine(const QaplaHelpers::IniFile::Section &section)
{
    std::optional<std::string> name = section.getValue("name");
	std::optional<std::string> indexStr = section.getValue("index");
    std::optional<size_t> index;

    if (indexStr) {
        try {
            index = std::stoul(*indexStr);
        } catch (const std::exception& e) {
            index = std::nullopt; 
        }
    }

    if (name && index) {
        auto config = EngineWorkerFactory::getConfigManager().getConfig(*name);
        if (*index >= engineConfigs_.size()) {
            engineConfigs_.resize(*index + 1);
        }
        engineConfigs_[*index] = *config;
    } else if (name) {
        auto config = EngineWorkerFactory::getConfigManager().getConfig(*name);
        engineConfigs_.push_back(*config);
    }
}

void InteractiveBoardWindow::doMove(const MoveRecord& move)
{
	computeTask_->doMove(move);
}

void InteractiveBoardWindow::setPosition(bool startPosition, const std::string &fen)
{
	computeTask_->setPosition(startPosition, fen);
}

void InteractiveBoardWindow::execute(std::string command)
{
	if (command == "New" && gameRecord_ != nullptr)
	{
		computeTask_->setPosition(true, "");
	}
	else if (command == "Stop")
	{
		computeTask_->stop();
	}
	else if (command == "Now")
	{
		computeTask_->moveNow();
	}
	else if (command == "Newgame")
	{
		computeTask_->newGame();
	}
	else if (command == "Play")
	{
		computeTask_->playSide();
	}
	else if (command == "Analyze")
	{
		computeTask_->analyze();
	}
	else if (command == "Auto")
	{
		computeTask_->autoPlay();
	}
	else if (command == "Manual")
	{
		computeTask_->stop();
	}
	else
	{
		std::cerr << "Unknown command: " << command << '\n';
	}
}

void InteractiveBoardWindow::stopPool()
{
	GameManagerPool::getInstance().stopAll();
}

void InteractiveBoardWindow::clearPool()
{
	GameManagerPool::getInstance().clearAll();
}

void InteractiveBoardWindow::setPoolConcurrency(uint32_t count, bool nice, bool start)
{
	GameManagerPool::getInstance().setConcurrency(count, nice, start);
}

void InteractiveBoardWindow::pollData()
{
	try
	{
		computeTask_->getGameContext().withGameRecord([&](const GameRecord &g) {
			setGameIfDifferent(g);
			imGuiMoveList_->setFromGameRecord(g);
			imGuiClock_->setFromGameRecord(g);
			timeControl_ = g.getWhiteTimeControl(); 
		});
		imGuiEngineList_->setAllowInput(true);
		computeTask_->getGameContext().withMoveRecord([&](const MoveRecord &m, uint32_t idx) {
			imGuiEngineList_->setFromMoveRecord(m, idx);
			imGuiClock_->setFromMoveRecord(m, idx);
		});
		computeTask_->getGameContext().withEngineRecords([&](const EngineRecords &records) {
			imGuiEngineList_->setEngineRecords(records);
		});
		epdData_.pollData();
		auto timeControl = QaplaConfiguration::Configuration::instance()
							   .getTimeControlSettings()
							   .getSelectedTimeControl();
		if (timeControl != timeControl_)
		{
			computeTask_->setTimeControl(timeControl);
		}
	}
	catch (const std::exception &ex)
	{
		assert(false && "Error while polling data");
	}
}

void InteractiveBoardWindow::setGameIfDifferent(const GameRecord &record)
{
	if (gameRecord_ == nullptr || record.isUpdate(*gameRecord_))
	{
		*gameRecord_ = record;
		imGuiBoard_->setGameState(*gameRecord_);
		auto [cause, result] = gameRecord_->getGameResult();
		imGuiBoard_->setAllowMoveInput(result == GameResult::Unterminated);
	}
}

uint32_t InteractiveBoardWindow::nextMoveIndex() const
{
	return gameRecord_->nextMoveIndex();
}

void InteractiveBoardWindow::setNextMoveIndex(uint32_t moveIndex)
{
	if (!gameRecord_)
	{
		return;
	}
	if (moveIndex <= gameRecord_->history().size())
	{
		gameRecord_->setNextMoveIndex(moveIndex);
		imGuiBoard_->setGameState(*gameRecord_);
		computeTask_->setPosition(*gameRecord_);
	}
}

bool InteractiveBoardWindow::isGameOver() const
{
	auto [cause, result] = gameRecord_->getGameResult();
	return result != GameResult::Unterminated &&
		   gameRecord_->nextMoveIndex() >= gameRecord_->history().size();
}

void InteractiveBoardWindow::stopEngine(const std::string &id)
{
	computeTask_->stopEngine(id);
}

void InteractiveBoardWindow::restartEngine(const std::string &id)
{
	computeTask_->restartEngine(id);
}

void InteractiveBoardWindow::setEngines(const std::vector<EngineConfig> &engines)
{
	engineConfigs_ = engines;
	if (engines.size() == 0)
	{
		computeTask_->initEngines(EngineList{});
		return;
	}
	auto created = EngineWorkerFactory::createEngines(engines, true);
	computeTask_->initEngines(std::move(created));
}

bool InteractiveBoardWindow::isModeActive(const std::string &mode) const
{
	auto status = computeTask_->getStatus();
	switch (status)
	{
	case ComputeTask::Status::Stopped:
		return mode == "Stop";
	case ComputeTask::Status::Play:
		return mode == "Play";
	case ComputeTask::Status::Autoplay:
		return mode == "Auto";
	case ComputeTask::Status::Analyze:
		return mode == "Analyze";
	default:
		return false;
	}
}

