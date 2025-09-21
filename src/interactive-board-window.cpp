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

#include "snackbar.h"
#include "imgui-clock.h"
#include "imgui-move-list.h"
#include "imgui-barchart.h"
#include "imgui-popup.h"
#include "horizontal-split-container.h"
#include "vertical-split-container.h"
#include "board-window.h"
#include "engine-window.h"
#include "engine-setup-window.h"
#include "imgui-cut-paste.h"
#include "game-parser.h"

#include "GLFW/glfw3.h"

using namespace QaplaWindows;

InteractiveBoardWindow::InteractiveBoardWindow(uint32_t id)
	: id_(id),
	  gameRecord_(std::make_unique<GameRecord>()),
	  computeTask_(std::make_unique<ComputeTask>()),
	  boardWindow_(std::make_unique<BoardWindow>()),
	  engineWindow_(std::make_unique<EngineWindow>()),
	  setupWindow_(std::make_unique<ImGuiPopup<EngineSetupWindow>>(
        ImGuiPopup<EngineSetupWindow>::Config{ .title = "Select Engines" })),
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

InteractiveBoardWindow::~InteractiveBoardWindow() {
	QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
		"engineselection",
		"board" + std::to_string(id_),
		{}
	);
};

std::unique_ptr<InteractiveBoardWindow> InteractiveBoardWindow::createInstance() {
	static uint32_t id = 1;
	auto instance = std::make_unique<InteractiveBoardWindow>(id);
	++id;

	instance->pollCallbackHandle_ = std::move(StaticCallbacks::poll().registerCallback(
		[instance = instance.get()]() {
			instance->pollData();
		}
	));

	return instance;
}

std::vector<std::unique_ptr<InteractiveBoardWindow>> InteractiveBoardWindow::loadInstances() {
	auto& config = QaplaConfiguration::Configuration::instance().getConfigData();
	auto sectionMap = config.getSectionMap("engineselection");
	if (!sectionMap) return {};
	std::vector<std::unique_ptr<InteractiveBoardWindow>> instances;
	for (const auto& [idStr, sectionList] : *sectionMap) {
		try {
			if (idStr == "board0") continue; // Skip static instance
			auto instance = createInstance();
			for (const auto& section : sectionList) {
				instance->loadBoardEngine(section);
			}
			instance->setEngines();
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
				auto move = boardWindow_->draw();
				if (move) {
					move->timeMs = imGuiClock_->getCurrentTimerMs();
					move->engineName_ = "Human";
					doMove(*move);
				}
			}
		);
        BoardMovesContainer->setRight(std::move(ClockMovesContainer));
        BoardMovesContainer->setPresetWidth(400.0f, false);

        auto BoardEngineContainer = std::make_unique<QaplaWindows::VerticalSplitContainer>("board_engine");
        BoardEngineContainer->setTop(std::move(BoardMovesContainer));
        BoardEngineContainer->setBottom(
			[this]() {
				auto [id, command] = engineWindow_->draw();
				drawEngineSelectionPopup();
				if (command == "") return;
				if (command == "Restart") restartEngine(id);
        		else if (command == "Stop") stopEngine(id);
				else if (command == "Config") openEngineSelectionPopup();
				else if (command == "Swap") swapEngines();
				else if (command.starts_with("pv|")) copyPv(id, command);
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

	// Handle paste only for the active tab
	GLFWwindow* window = glfwGetCurrentContext();
	if (window) {
		auto pasted = ImGuiCutPaste::checkForPaste(window);
		if (pasted) {
			auto gameRecord = QaplaUtils::GameParser().parse(*pasted);
			if (gameRecord) {
				setPosition(*gameRecord);
			}
		}
	}
}

void InteractiveBoardWindow::copyPv(const std::string& id, const std::string& pv) {
	// Expected format produced by encodePV: "pv|<halfmoveNo>|<pv...>"

	std::string_view sv(pv);
	constexpr std::string_view prefix = "pv|";
	if (!sv.starts_with(prefix)) return;

	// skip prefix
	sv.remove_prefix(prefix.size());

	// find separator between number and pv text
	size_t sep = sv.find('|');
	if (sep == std::string_view::npos) return;

	std::string_view numPart = sv.substr(0, sep);
	std::string_view pvPart = sv.substr(sep + 1);

	// parse unsigned integer without exceptions
	uint32_t halfmove = 0;
	if (!numPart.empty()) {
		// use from_chars for fast, no-throw parsing
		auto [ptr, ec] = std::from_chars(numPart.data(), numPart.data() + numPart.size(), halfmove);
		if (ec != std::errc()) return; // parse failed
	} else {
		return; // no number
	}

	std::string pvString;
	computeTask_->getGameContext().withGameRecord([&](const GameRecord &g) {
		if (halfmove == 0) return;
		auto ply = g.getHalfmoveIndex(halfmove - 1);
		if (!ply) return;
		pvString = g.movesToStringUpToPly(*ply, {true, true, true, true}) + " ";
	});

	// convert pvPart to std::string for clipboard
	pvString += std::string(pvPart);

	GLFWwindow* window = glfwGetCurrentContext();
	if (window) {
		ImGuiCutPaste::setClipboardString(window, pvString);
		SnackbarManager::instance().showNote("Copied PV to clipboard:\n" + pvString);
	}
}

void InteractiveBoardWindow::swapEngines() {
	if (engineConfigs_.size() < 2) return;
	bool isSwitched = computeTask_->getGameContext().isSideSwitched();
	computeTask_->getGameContext().setSideSwitched(!isSwitched);
}

void InteractiveBoardWindow::openEngineSelectionPopup() {
	// Open engine setup window
	std::vector<EngineConfig> activeEngines;
	for (const auto& record : engineWindow_->getEngineRecords()) {
		activeEngines.push_back(record.config);
	}
	setupWindow_->content().setMatchingActiveEngines(activeEngines);
    setupWindow_->open();
}

void InteractiveBoardWindow::drawEngineSelectionPopup() {
	if (!setupWindow_) return;
	setupWindow_->draw("Use", "Cancel");
    if (auto confirmed = setupWindow_->confirmed()) {
        if (*confirmed) {
			setEngines(setupWindow_->content().getActiveEngines());
        }
        setupWindow_->resetConfirmation();
    }
}

QaplaHelpers::IniFile::SectionList InteractiveBoardWindow::getIniSections() const {
	QaplaHelpers::IniFile::SectionList sections;
	uint32_t index = 0;
	for (const auto& engine : engineConfigs_) {
		QaplaHelpers::IniFile::Section section;
		section.name = "engineselection";
		section.addEntry("id", "board" + std::to_string(id_));
		section.addEntry("name", engine.getName());
		section.addEntry("index", std::to_string(index));
		sections.push_back(section);
		++index;
	}
	return sections;
}

void InteractiveBoardWindow::saveConfig(std::ostream &out) const
{
	auto sections = getIniSections();
	for (const auto& section : sections) {
		QaplaHelpers::IniFile::saveSection(out, section);
	}
}

bool InteractiveBoardWindow::loadBoardEngine(const QaplaHelpers::IniFile::Section &section)
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
		if (!config) return false;
        if (*index >= engineConfigs_.size()) {
            engineConfigs_.resize(*index + 1);
        }
        engineConfigs_[*index] = *config;
    } else if (name) {
        auto config = EngineWorkerFactory::getConfigManager().getConfig(*name);
		if (!config) return false;
        engineConfigs_.push_back(*config);
    } else {
		return false;
	}
	return true;
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
	//imGuiClock_->setStopped(true);
}

void InteractiveBoardWindow::playSide()
{
	try {
		computeTask_->playSide();
		//imGuiClock_->setStopped(false);
	}
	catch (const std::exception& e) {
		SnackbarManager::instance().showError(std::string("Failed to compute a move:\n") + e.what());
	}	
}

void InteractiveBoardWindow::analyze()
{
	try {
		computeTask_->analyze();
		//imGuiClock_->setStopped(false);
	}
	catch (const std::exception& e) {
		SnackbarManager::instance().showError(std::string("Failed to analyze:\n") + e.what());
	}
}

void InteractiveBoardWindow::autoPlay()
{
	try {
		computeTask_->autoPlay();
		//imGuiClock_->setStopped(false);
	}
	catch (const std::exception& e) {
		SnackbarManager::instance().showError(std::string("Failed to compute moves:\n") + e.what());
	}
}

void InteractiveBoardWindow::setStartPosition()
{
	//imGuiClock_->setStopped(true);
	computeTask_->setPosition(true, "");
}

void InteractiveBoardWindow::execute(std::string command)
{
	if (command == "") return;

	if (command == "New") setStartPosition();
	else if (command == "Stop") stop();
	else if (command == "Now") computeTask_->moveNow();
	else if (command == "Newgame") computeTask_->newGame();
	else if (command == "Play") playSide();
	else if (command == "Analyze") analyze();
	else if (command == "Auto") autoPlay();
	else if (command == "Invert") boardWindow_->setInverted(!boardWindow_->isInverted());
	else std::cerr << "Unknown command: " << command << '\n';
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
		if (computeTask_->isStopped()) {
			imGuiClock_->setStopped(true);
		}
		else {
			imGuiClock_->setStopped(false);
		}
		imGuiClock_->setAnalyze(computeTask_->getStatus() == "Analyze");
		computeTask_->getGameContext().withGameRecord([&](const GameRecord &g) {
			imGuiMoveList_->setFromGameRecord(g);
			imGuiClock_->setFromGameRecord(g);
			imGuiBarChart_->setFromGameRecord(g);
			boardWindow_->setFromGameRecord(g);
			engineWindow_->setFromGameRecord(g);
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
	// Inform global configuration about the change
	QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
		"engineselection",
		"board" + std::to_string(id_),
		getIniSections()
	);
	if (engines.size() == 0)
	{
		computeTask_->initEngines(EngineList{});
		return;
	}
	auto created = EngineWorkerFactory::createEngines(engines, true);
	computeTask_->initEngines(std::move(created));
}



