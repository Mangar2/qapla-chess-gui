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
#include "snackbar.h"
#include "imgui-clock.h"
#include "imgui-move-list.h"
#include "imgui-barchart.h"
#include "imgui-popup.h"
#include "imgui-game-list.h"
#include "horizontal-split-container.h"
#include "vertical-split-container.h"
#include "board-window.h"
#include "engine-window.h"
#include "engine-setup-window.h"
#include "time-control-window.h"
#include "imgui-cut-paste.h"
#include "game-parser.h"
#include "epd-data.h"
#include "callback-manager.h"

#include <string-helper.h>
#include <qapla-engine/move.h>
#include <time-control.h>
#include <game-state.h>
#include <game-record.h>
#include <compute-task.h>
#include <engine-config.h>
#include <engine-config-manager.h>
#include <engine-worker-factory.h>
#include <game-manager-pool.h>

#include <GLFW/glfw3.h>

using namespace QaplaTester;

using namespace QaplaWindows;

InteractiveBoardWindow::InteractiveBoardWindow(uint32_t id)
	: gameRecord_(std::make_unique<GameRecord>()),
	  computeTask_(std::make_unique<ComputeTask>()),
	  boardWindow_(std::make_unique<BoardWindow>()),
	  engineWindow_(std::make_unique<EngineWindow>()),
	  setupWindow_(std::make_unique<ImGuiPopup<EngineSetupWindow>>(
		ImGuiPopup<EngineSetupWindow>::Config{ 
			.title = "Select Engines",
			.okButton = true,
			.cancelButton = false
		},
		ImVec2(600, 600))
		),
	  timeControlWindow_(std::make_unique<ImGuiPopup<TimeControlWindow>>(
		ImGuiPopup<TimeControlWindow>::Config{
			.title = "Time Control Configuration",
			.okButton = true,
			.cancelButton = true
		},
		ImVec2(500, 500))
		),
	  imGuiClock_(std::make_unique<ImGuiClock>()),
	  imGuiMoveList_(std::make_unique<ImGuiMoveList>()),
	  imGuiBarChart_(std::make_unique<ImGuiBarChart>()),
	  id_(id)
{
	timeControlWindow_->content().setFromConfiguration("board" + std::to_string(id_));
	timeControl_ = timeControlWindow_->content().getSelectedTimeControl();
	computeTask_->setTimeControl(timeControl_);
	setupWindow_->content().setDirectEditMode(false);
	setupWindow_->content().setAllowMultipleSelection(true);
	setupWindow_->content().setShowButtons(false);
	setupWindow_->content().setId("board" + std::to_string(id_));
	setPosition(true);
	initSplitterWindows();
}

InteractiveBoardWindow::~InteractiveBoardWindow() {
	timeControlWindow_->content().updateConfiguration("board" + std::to_string(id_));
	QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
		"engineselection",
		"board" + std::to_string(id_),
		{}
	);
};

std::unique_ptr<InteractiveBoardWindow> InteractiveBoardWindow::createInstance() {
	static uint32_t id = 1;
	auto instance = std::make_unique<InteractiveBoardWindow>(id);
	instance->saveCallbackHandle_ = StaticCallbacks::save().registerCallback([instancePtr = instance.get()]() {
		instancePtr->save();
	});
	++id;
	return instance;
}

void InteractiveBoardWindow::loadGlobalEngineConfiguration(const std::string &idStr)
{
	auto& config = QaplaConfiguration::Configuration::instance().getConfigData();
	auto globalSettings = config.getSectionList("eachengine", idStr);
	if (globalSettings) {
		setupWindow_->content().setGlobalConfiguration(*globalSettings);
	}
}

void InteractiveBoardWindow::loadBoardEnginesConfiguration(
	const QaplaHelpers::IniFile::SectionList &sectionList)
{
	setupWindow_->content().setEnginesConfiguration(sectionList);
	setEngines(setupWindow_->content().getActiveEngines());
}

std::vector<std::unique_ptr<InteractiveBoardWindow>> InteractiveBoardWindow::loadInstances() {
	auto& config = QaplaConfiguration::Configuration::instance().getConfigData();
	auto sectionMap = config.getSectionMap("engineselection");
	if (!sectionMap) {
		return {};
	}
	
	std::vector<std::unique_ptr<InteractiveBoardWindow>> instances;
	for (const auto& [idStr, sectionList] : *sectionMap) {
		try {
			if (idStr == "board0") {
				continue; // Skip static instance
			}
			if (!idStr.starts_with("board")) {
				continue;
			}
			auto instance = createInstance();
            instance->loadGlobalEngineConfiguration(idStr);
			instance->loadBoardEnginesConfiguration(sectionList);
			instances.push_back(std::move(instance));
		} catch (const std::exception& e) {
			// Ignore invalid entries
		}
	}
	if (instances.empty()) {
		instances.push_back(createInstance());
	}
	return instances;
}



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
		MovesBarchartContainer->setPresetHeight(180.0F, false);

        auto ClockMovesContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>("clock_moves");
        ClockMovesContainer->setFixedHeight(120.0F, true);
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
				drawTimeControlPopup();
				auto move = boardWindow_->draw();
				if (move) {
					move->timeMs = imGuiClock_->getCurrentTimerMs();
					move->engineName_ = "Human";
					doMove(*move);
				}
			}
		);
        BoardMovesContainer->setRight(std::move(ClockMovesContainer));
        BoardMovesContainer->setPresetWidth(400.0F, false);

        auto BoardEngineContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>("board_engine");
        BoardEngineContainer->setTop(std::move(BoardMovesContainer));
        BoardEngineContainer->setBottom(
			[this]() {
				auto [id, command] = engineWindow_->draw();
				drawEngineSelectionPopup();
				if (command.empty()) {
					return;
				}
				if (command == "Restart") {
					restartEngine(id);
				}
				else if (command == "Stop") {
					stopEngine(id);
				}
				else if (command == "Config") {
					setupWindow_->open();
				}
				else if (command == "Swap") {
					swapEngines();
				}
				else if (command.starts_with("pv|")) {
					copyPv(command);
				}
			}
		);
        BoardEngineContainer->setMinBottomHeight(55.0F);
        BoardEngineContainer->setPresetHeight(230.0F, false);

		mainWindow_ = std::move(BoardEngineContainer);
}

void InteractiveBoardWindow::draw() {
	if (!mainWindow_) {
		return;
	}
	mainWindow_->draw();
	
	pollData();

	// Handle paste only for the active tab
	GLFWwindow* window = glfwGetCurrentContext();
	if (window != nullptr) {
		auto pasted = ImGuiCutPaste::checkForPaste();
		if (pasted) {
			auto gameRecord = QaplaUtils::GameParser().parse(*pasted);
			if (gameRecord) {
				setPosition(*gameRecord);
			}
		}
	}
}

void InteractiveBoardWindow::drawTimeControlPopup() {
	if (!timeControlWindow_) {
		return;
	}
	
	timeControlWindow_->draw("Apply", "Cancel");
	if (auto confirmed = timeControlWindow_->confirmed()) {
		if (*confirmed) {
			// Apply changes
			timeControl_ = timeControlWindow_->content().getSelectedTimeControl();
			computeTask_->setTimeControl(timeControl_);
			timeControlWindow_->content().updateConfiguration("board" + std::to_string(id_));
			// Notify tutorial that Time Control popup was confirmed
			if (boardWindow_) {
				boardWindow_->showNextCutPasteTutorialStep("Time Control Confirmed");
			}
		}
		else {
			// Cancel - reload from configuration (discards changes)
			timeControlWindow_->content().setFromConfiguration("board" + std::to_string(id_));
		}
		timeControlWindow_->resetConfirmation();
	}
}

std::string InteractiveBoardWindow::computePgn(uint32_t upToHalfmove) {
	std::string pgnString;
	computeTask_->getGameContext().withGameRecord([&](const GameRecord &gameRecord) {
		if (!gameRecord.getStartPos() && !gameRecord.getStartFen().empty()) {
			pgnString = "[FEN \"" + gameRecord.getStartFen() + "\"]\n";
		}
		
		// Determine the last ply to include
		uint32_t targetHalfmove = upToHalfmove;
		if (targetHalfmove == 0) {
			targetHalfmove = gameRecord.nextMoveIndex() + 1;
		}
		
		auto ply = gameRecord.getHalfmoveIndex(targetHalfmove - 1);
		if (ply) {
			pgnString += gameRecord.movesToStringUpToPly(*ply, MoveRecord::toStringOptions{
				.includeClock = true,
				.includeEval = true,
				.includePv = true,
				.includeDepth = true
			});
		}
	});
	return pgnString;
}

void InteractiveBoardWindow::savePgnGame() const
{
	computeTask_->getGameContext().withGameRecord([&](const QaplaTester::GameRecord& gameRecord) {
		// Only save if game has at least one move or a custom FEN position
		bool hasCustomFen = !gameRecord.getStartPos();
		bool hasMoves = !gameRecord.history().empty();
		
		if (hasCustomFen || hasMoves) {
			PgnAutoSaver::instance().addGame(gameRecord);
		}
	});
}

void InteractiveBoardWindow::copyPv(const std::string& pv) {
	// Expected format produced by encodePV: "pv|<halfmoveNo>|<pv...>"

	std::string_view sv(pv);
	constexpr std::string_view prefix = "pv|";
	if (!sv.starts_with(prefix)) {
		return;
	}

	// skip prefix
	sv.remove_prefix(prefix.size());

	// find separator between number and pv text
	size_t sep = sv.find('|');
	if (sep == std::string_view::npos) {
		return;
	}

	std::string_view numPart = sv.substr(0, sep);
	std::string_view pvPart = sv.substr(sep + 1);

	// parse unsigned integer without exceptions
	auto halfmove = QaplaHelpers::to_uint32(numPart);
	if (!halfmove) {
		return; // parse failed
	}

	std::string pvString = computePgn(*halfmove);
	pvString += std::string(pvPart);

	GLFWwindow* window = glfwGetCurrentContext();
	if (window != nullptr) {
		ImGuiCutPaste::setClipboardString(pvString);
		SnackbarManager::instance().showNote("Copied PV to clipboard:\n" + pvString);
	}
}

void InteractiveBoardWindow::swapEngines() {
	bool isSwitched = computeTask_->getGameContext().isSideSwitched();
	computeTask_->getGameContext().setSideSwitched(!isSwitched);
}

void InteractiveBoardWindow::drawEngineSelectionPopup() {
	if (!setupWindow_) {
		return;
	}
	setupWindow_->draw("Use", "Cancel");
    if (auto confirmed = setupWindow_->confirmed()) {
        if (*confirmed) {
			setEngines(setupWindow_->content().getActiveEngines());
        }
        setupWindow_->resetConfirmation();
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

void InteractiveBoardWindow::setPosition(const GameRecord &gameRecord)
{
	computeTask_->setPosition(gameRecord);
}

void InteractiveBoardWindow::stop()
{
	computeTask_->stop();
}

void InteractiveBoardWindow::playSide()
{
	try {
		computeTask_->playSide();
	}
	catch (const std::exception& e) {
		SnackbarManager::instance().showError(std::string("Failed to compute a move:\n") + e.what());
	}	
}

void InteractiveBoardWindow::analyze()
{
	try {
		computeTask_->analyze();
	}
	catch (const std::exception& e) {
		SnackbarManager::instance().showError(std::string("Failed to analyze:\n") + e.what());
	}
}

void InteractiveBoardWindow::autoPlay()
{
	try {
		computeTask_->autoPlay();
	}
	catch (const std::exception& e) {
		SnackbarManager::instance().showError(std::string("Failed to compute moves:\n") + e.what());
	}
}

void InteractiveBoardWindow::openTimeControlDialog()
{
	timeControlWindow_->open();
}

void InteractiveBoardWindow::execute(const std::string& command)
{
	if (command.empty()) {
		return;
	}

	if (command == "New") {
		// Order is important because newGame sets the current position for winboard
		savePgnGame();
		setPosition(true, "");
		computeTask_->newGame();
	}
	else if (command == "Stop") {
		stop();
	}
	else if (command == "Now") {
		computeTask_->moveNow();
	}
	else if (command == "Newgame") {
		computeTask_->newGame();
	}
	else if (command == "Play") {
		playSide();
	}
	else if (command == "Analyze") {
		analyze();
	}
	else if (command == "Auto") {
		autoPlay();
	}
	else if (command == "Invert") {
		boardWindow_->setInverted(!boardWindow_->isInverted());
	}
	else if (command == "Paste") {
		auto pasted = ImGuiCutPaste::getClipboardString();
		if (pasted) {
			auto gameRecord = QaplaUtils::GameParser().parse(*pasted);
			if (gameRecord) {
				savePgnGame();
				setPosition(*gameRecord);
			}
		}
	}
	else if (command == "Copy PGN") {
		auto pgn = computePgn();
		ImGuiCutPaste::setClipboardString(pgn);
		SnackbarManager::instance().showNote("PGN copied to clipboard\n" + pgn);
	}
	else if (command == "Copy FEN") {
		auto fen = boardWindow_->getFen();
		ImGuiCutPaste::setClipboardString(fen);
		SnackbarManager::instance().showNote("FEN copied to clipboard\n" + fen);
	}
	else if (command == "Time") {
		openTimeControlDialog();
	}
	else if (command.starts_with("Position: ")) {
		savePgnGame();
		computeTask_->newGame();
		std::string fen = command.substr(std::string("Position: ").size());
		setPosition(false, fen);
	}
	else if (command == "More") {
		// Handled in board window
	} else {
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
	if (engines.empty())
	{
		computeTask_->initEngines(EngineList{});
		return;
	}
	auto created = EngineWorkerFactory::createEngines(engines, true);
	computeTask_->initEngines(std::move(created));
}

void InteractiveBoardWindow::pollData()
{
	try
	{
		if (ImGuiGameList::getSelectedGame()) {
			setPosition(*ImGuiGameList::getSelectedGame());
		}
		if (EpdData::instance().getSelectedIndex()) {
			auto index = *EpdData::instance().getSelectedIndex();
			setPosition(false, *EpdData::instance().getFen(index));
		}
		if (computeTask_->isStopped()) {
			imGuiClock_->setStopped(true);
		}
		else {
			imGuiClock_->setStopped(false);
		}
		imGuiClock_->setAnalyze(computeTask_->getStatus() == "Analyze");
		computeTask_->getGameContext().withGameRecord([&](const GameRecord &gameRecord) {
			imGuiMoveList_->setFromGameRecord(gameRecord);
			imGuiClock_->setFromGameRecord(gameRecord);
			imGuiBarChart_->setFromGameRecord(gameRecord);
			boardWindow_->setFromGameRecord(gameRecord);
			engineWindow_->setFromGameRecord(gameRecord);
			timeControl_ = gameRecord.getWhiteTimeControl(); 
		});
		engineWindow_->setAllowInput(true);
		computeTask_->getGameContext().withMoveRecord([&](const MoveRecord &moveRecord, uint32_t idx) {
			engineWindow_->setFromMoveRecord(moveRecord, idx, computeTask_->getStatus());
			imGuiClock_->setFromMoveRecord(moveRecord, idx);
		});
		const std::string engineId = "";
		engineWindow_->pollLogBuffers();
		computeTask_->getGameContext().withEngineRecords([&](const EngineRecords &records) {
			engineWindow_->setEngineRecords(records);
		});
	}
	catch (const std::exception &ex)
	{
		assert(false && "Error while polling data");
	}
}




