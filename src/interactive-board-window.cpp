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

#include "imgui-clock.h"
#include "imgui-move-list.h"
#include "imgui-barchart.h"
#include "horizontal-split-container.h"
#include "vertical-split-container.h"
#include "board-window.h"
#include "engine-window.h"

using namespace QaplaWindows;

InteractiveBoardWindow::InteractiveBoardWindow()
	: gameRecord_(std::make_unique<GameRecord>()),
	  computeTask_(std::make_unique<ComputeTask>()),
	  boardWindow_(std::make_unique<BoardWindow>()),
	  engineWindow_(std::make_unique<EngineWindow>()),
	  imGuiClock_(std::make_unique<ImGuiClock>()),
	  imGuiMoveList_(std::make_unique<ImGuiMoveList>()),
	  imGuiBarChart_(std::make_unique<ImGuiBarChart>())
{
	timeControl_ = QaplaConfiguration::Configuration::instance()
					   .getTimeControlSettings()
					   .getSelectedTimeControl();
	computeTask_->setTimeControl(timeControl_);
	computeTask_->setPosition(true);
	initSplitterWindows();
	epdData_.init();
}

InteractiveBoardWindow::~InteractiveBoardWindow() = default;

void InteractiveBoardWindow::initSplitterWindows()
{
		auto MovesBarchartContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>("moves_barchart");
		imGuiMoveList_->setClickable(true);
		MovesBarchartContainer->setTop(
			[this]() {
				auto selected = imGuiMoveList_->draw();
				if (selected) {
					setNextMoveIndex(static_cast<uint32_t>(*selected));
				}
			}
		);
		MovesBarchartContainer->setBottom(
			[this]() {
				auto clicked = imGuiBarChart_->draw();
				if (clicked) {
					setNextMoveIndex(static_cast<uint32_t>(*clicked));
				}
			}
		);
		MovesBarchartContainer->setPresetHeight(180.0f, false);

        auto ClockMovesContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>("clock_moves");
        ClockMovesContainer->setFixedHeight(120.0f, true);
        ClockMovesContainer->setTop(
			[this]() {
				imGuiClock_->draw();
			}
		);
        ClockMovesContainer->setBottom(std::move(MovesBarchartContainer));

        auto BoardMovesContainer = std::make_unique<QaplaWindows::HorizontalSplitContainer>("board_moves");
		boardWindow_->setAllowMoveInput(true);
        BoardMovesContainer->setLeft(
			[this]() {
				auto command = boardWindow_->drawButtons(computeTask_->getStatus());
				execute(command);
				const auto move = boardWindow_->draw();
				if (move) doMove(*move);
			}
		);
        BoardMovesContainer->setRight(std::move(ClockMovesContainer));
        BoardMovesContainer->setPresetWidth(400.0f, false);

        auto BoardEngineContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>("board_engine");
        BoardEngineContainer->setTop(std::move(BoardMovesContainer));
        BoardEngineContainer->setBottom(
			[this]() {
				auto [id, command] = engineWindow_->draw();
				if (command == "Restart") restartEngine(id);
        		else if (command == "Stop") stopEngine(id);
			}
		);
        BoardEngineContainer->setMinBottomHeight(55.0f);
        BoardEngineContainer->setPresetHeight(230.0f, false);

		mainWindow_ = std::move(BoardEngineContainer);
}

void InteractiveBoardWindow::draw() {
	if (mainWindow_) {
		mainWindow_->draw();
	}
}

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
	if (command == "") return;

	if (command == "New")
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
	else if (command == "Invert")
	{
		boardWindow_->setInverted(!boardWindow_->isInverted());
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
			imGuiMoveList_->setFromGameRecord(g);
			imGuiClock_->setFromGameRecord(g);
			imGuiBarChart_->setFromGameRecord(g);
			boardWindow_->setFromGameRecord(g);
			timeControl_ = g.getWhiteTimeControl(); 
		});
		engineWindow_->setAllowInput(true);
		computeTask_->getGameContext().withMoveRecord([&](const MoveRecord &m, uint32_t idx) {
			engineWindow_->setFromMoveRecord(m, idx);
			imGuiClock_->setFromMoveRecord(m, idx);
		});
		computeTask_->getGameContext().withEngineRecords([&](const EngineRecords &records) {
			engineWindow_->setEngineRecords(records);
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

void InteractiveBoardWindow::setNextMoveIndex(uint32_t moveIndex)
{
	computeTask_->setNextMoveIndex(moveIndex);
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



