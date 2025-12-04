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

#include "sprt-tournament-data.h"
#include "configuration.h"
#include "snackbar.h"
#include "imgui-concurrency.h"

#include "ini-file.h"
#include "string-helper.h"
#include "sprt-manager.h"
#include "engine-option.h"
#include "pgn-io.h"
#include "pgn-save.h"
#include "game-manager-pool.h"

#include <algorithm>
#include <format>

using namespace QaplaTester;
using namespace QaplaWindows;

SprtTournamentData::SprtTournamentData() : 
    boardWindowList_("SPRT Tournament"),
    resultTable_(
            "TournamentResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Engine in Test", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Engine to Compare", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Rating", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                { .name = "Games", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true }
            }
        ),
    sprtTable_(
            "SprtResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Engine in Test", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Engine to Compare", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 150.0F },
                { .name = "Result", .flags = ImGuiTableColumnFlags_WidthStretch }
            }
        ),
    montecarloTable_(
            "MonteCarloResult",
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
            std::vector<ImGuiTable::ColumnDef>{
                { .name = "Elo Diff", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F, .alignRight = true },
                { .name = "No Decision %", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F, .alignRight = true },
                { .name = "H0 Accepted %", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F, .alignRight = true },
                { .name = "H1 Accepted %", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 120.0F, .alignRight = true },
                { .name = "Avg Games", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 100.0F, .alignRight = true }
            }
        ),
    engineSelect_(std::make_unique<ImGuiEngineSelect>()),
    tournamentOpening_(std::make_unique<ImGuiTournamentOpening>()),
    tournamentPgn_(std::make_unique<ImGuiTournamentPgn>()),
    tournamentAdjudication_(std::make_unique<ImGuiTournamentAdjudication>()),
    globalSettings_(std::make_unique<ImGuiEngineGlobalSettings>()),
    sprtManager_(std::make_unique<SprtManager>()),
    sprtConfig_(std::make_unique<SprtConfig>()),
    imguiConcurrency_(std::make_unique<ImGuiConcurrency>())
{
    ImGuiEngineSelect::Options options;
    options.allowGauntletEdit = true;
    options.allowPonderEdit = true;
    options.allowTimeControlEdit = true;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = true;
    options.allowMultipleSelection = true;
    engineSelect_->setOptions(options);

    tournamentOpening_->setId("sprt-tournament");
    tournamentPgn_->setId("sprt-tournament");
    tournamentAdjudication_->setId("sprt-tournament");
    globalSettings_->setId("sprt-tournament");

    sprtConfig_->eloLower = -5;
    sprtConfig_->eloUpper = 5;
    sprtConfig_->alpha = 0.05;
    sprtConfig_->beta = 0.05;
    sprtConfig_->maxGames = 100000;

    setupCallbacks();
    loadEngineSelectionConfig();
    tournamentOpening_->loadConfiguration();
    tournamentPgn_->loadConfiguration();
    tournamentAdjudication_->loadConfiguration();
    loadSprtConfig();
    loadGlobalSettingsConfig();
    loadTournament();

    // Register poll callback
    pollCallbackHandle_ = StaticCallbacks::poll().registerCallback(
        [this]() {
            this->pollData();
        }
    );
}

SprtTournamentData::~SprtTournamentData() = default;


void SprtTournamentData::setGameManagerPool(const std::shared_ptr<GameManagerPool>& pool) {
    poolAccess_ = GameManagerPoolAccess(pool);
    boardWindowList_.setPoolAccess(poolAccess_);
    imguiConcurrency_->setPoolAccess(poolAccess_);
}

void SprtTournamentData::setupCallbacks() {
    engineSelect_->setConfigurationChangedCallback(
        [this](const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
            engineConfigurations_ = configurations;
        }
    );

    globalSettings_->setConfigurationChangedCallback(
        [this](const ImGuiEngineGlobalSettings::GlobalConfiguration& settings) {
            eachEngineConfig_ = settings;
        }
    );

    globalSettings_->setTimeControlChangedCallback(
        [this](const ImGuiEngineGlobalSettings::TimeControlSettings& settings) {
            timeControlSettings_ = settings;
        }
    );

    // Message callback to handle external messages
    messageCallbackHandle_ = StaticCallbacks::message().registerCallback(
        [this](const std::string& msg) {
            if (msg == "switch_to_sprt_view") {
                this->activateBoardView(0);
            }
        }
    );
}

void SprtTournamentData::setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations) {
    engineConfigurations_ = configurations;
}

void SprtTournamentData::loadEngineSelectionConfig() {
    auto sections = QaplaConfiguration::Configuration::instance()
        .getConfigData().getSectionList("engineselection", "sprt-tournament")
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    engineSelect_->setId("sprt-tournament");
    engineSelect_->setEnginesConfiguration(sections);
}

void SprtTournamentData::loadSprtConfig() {
    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto sections = configData.getSectionList("sprtconfig", "sprt-tournament");
    if (!sections || sections->empty()) {
        return;
    }

    for (const auto& [key, value] : (*sections)[0].entries) {
        if (key == "eloLower") {
            sprtConfig_->eloLower = QaplaHelpers::to_int(value).value_or(-5);
            sprtConfig_->eloLower = std::clamp(sprtConfig_->eloLower, -1000, 1000);
        }
        else if (key == "eloUpper") {
            sprtConfig_->eloUpper = QaplaHelpers::to_int(value).value_or(5);
            sprtConfig_->eloUpper = std::clamp(sprtConfig_->eloUpper, -1000, 1000);
        }
        else if (key == "alpha") {
            try {
                sprtConfig_->alpha = std::stod(value);
                sprtConfig_->alpha = std::clamp(sprtConfig_->alpha, 0.001, 0.5);
            } catch (...) {
                sprtConfig_->alpha = 0.05;
            }
        }
        else if (key == "beta") {
            try {
                sprtConfig_->beta = std::stod(value);
                sprtConfig_->beta = std::clamp(sprtConfig_->beta, 0.001, 0.5);
            } catch (...) {
                sprtConfig_->beta = 0.05;
            }
        }
        else if (key == "maxGames") {
            sprtConfig_->maxGames = QaplaHelpers::to_uint32(value).value_or(100000);
        }
    }
}

void SprtTournamentData::loadGlobalSettingsConfig() {
    auto& config = QaplaConfiguration::Configuration::instance();
    
    auto globalSections = config.getConfigData().getSectionList("eachengine", "sprt-tournament")
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    globalSettings_->setId("sprt-tournament");
    globalSettings_->setGlobalConfiguration(globalSections);
    
    auto timeControlSections = config.getConfigData().getSectionList("timecontroloptions", "sprt-tournament")
        .value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    globalSettings_->setTimeControlConfiguration(timeControlSections);
}

void SprtTournamentData::updateConfiguration() {
    QaplaHelpers::IniFile::KeyValueMap sprtEntries{
        {"id", "sprt-tournament"},
        {"eloLower", std::to_string(sprtConfig_->eloLower)},
        {"eloUpper", std::to_string(sprtConfig_->eloUpper)},
        {"alpha", std::to_string(sprtConfig_->alpha)},
        {"beta", std::to_string(sprtConfig_->beta)},
        {"maxGames", std::to_string(sprtConfig_->maxGames)}
    };

    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "sprtconfig", "sprt-tournament", {{
            .name = "sprtconfig",
            .entries = sprtEntries
        }});

    auto openingSections = tournamentOpening_->getSections();
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "opening", "sprt-tournament", openingSections);

    auto pgnSections = tournamentPgn_->getSections();
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "pgnoutput", "sprt-tournament", pgnSections);

    auto adjudicationSections = tournamentAdjudication_->getSections();
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "drawadjudication", "sprt-tournament", { adjudicationSections[0] });
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "resignadjudication", "sprt-tournament", { adjudicationSections[1] });
}

void SprtTournamentData::updateTournamentResults() {
    if (sprtManager_) {
        auto section = sprtManager_->getSection();
        if (section.has_value()) {
            QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
                "round", "sprt-tournament", { *section });
        } else {
            QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
                "round", "sprt-tournament", {});
        }
    }
}

SprtConfig& SprtTournamentData::sprtConfig() {
    return *sprtConfig_;
}

const SprtConfig& SprtTournamentData::sprtConfig() const {
    return *sprtConfig_;
}

bool SprtTournamentData::createTournament(bool verbose) {
    try {
        // Build engine configurations with global settings applied
        std::vector<EngineConfig> selectedEngines;
        for (auto& tournamentConfig : engineConfigurations_) {
            if (!tournamentConfig.selected) {
                continue;
            }
            EngineConfig engine = tournamentConfig.config;
            
            // Apply global settings to engine
            ImGuiEngineGlobalSettings::applyGlobalConfig(engine, eachEngineConfig_, timeControlSettings_);
            
            selectedEngines.push_back(engine);
        }
        if (selectedEngines.size() != 2) {
            throw std::runtime_error("SPRT tournament requires exactly 2 engines.");
        }

        // Set up SPRT config with openings
        sprtConfig_->openings = tournamentOpening_->openings();
        if (sprtConfig_->openings.file.empty()) {
            throw std::runtime_error("No openings file specified.");
        }

        // Set PGN output options and create tournament
        if (sprtManager_) {
            PgnSave::tournament().setOptions(tournamentPgn_->pgnOptions());
            QaplaTester::AdjudicationManager::poolInstance().setDrawAdjudicationConfig(tournamentAdjudication_->drawConfig());
            QaplaTester::AdjudicationManager::poolInstance().setResignAdjudicationConfig(tournamentAdjudication_->resignConfig());
            sprtManager_->createTournament(selectedEngines[0], selectedEngines[1], *sprtConfig_);
            
            // Clear Monte Carlo results when creating new tournament
            sprtManager_->clearMonteCarloResult();
        } else {
            throw std::runtime_error("Internal error, SPRT manager not initialized");
        }
    } catch (const std::exception& ex) {
        if (verbose) {
            SnackbarManager::instance().showError(std::string("Failed to create SPRT tournament:\n ") + ex.what());
        }
        return false;
    }
    return true;
}

void SprtTournamentData::loadTournament() {
    if (createTournament(false)) {
        auto sections = QaplaConfiguration::Configuration::instance().getConfigData().
            getSectionList("round", "sprt-tournament").value_or(std::vector<QaplaHelpers::IniFile::Section>{});

        if (sprtManager_ && !sections.empty()) {
            // SPRT typically has only one section (one pairing)
            sprtManager_->loadFromSection(sections[0]);
        }
    }
}

void SprtTournamentData::startTournament() {
    if (!sprtManager_) {
        SnackbarManager::instance().showError("Internal error, SPRT manager not initialized", 
            false, "sprt-tournament");
        return;
    }
    if (!createTournament(true)) {
        return;
    }
    if (isFinished()) {
        SnackbarManager::instance().showNote("Tournament already finished", 
            false, "sprt-tournament");
        return;
    }

    state_ = State::Starting;

    poolAccess_->clearAll();
    state_ = State::Starting;
    sprtManager_->schedule(sprtManager_, imguiConcurrency_->getExternalConcurrency(), *poolAccess_);
    imguiConcurrency_->init();
    imguiConcurrency_->setActive(true);
    state_ = State::Starting;
    SnackbarManager::instance().showSuccess("SPRT tournament started",
        false, "sprt-tournament");
}

void SprtTournamentData::pollData() {
    if (sprtManager_) {
        updateTournamentResults();
        populateResultTable();
        populateSprtTable();
        populateCausesTable();
        boardWindowList_.populateViews();
        populateMonteCarloTable();
        
        // Update state based on running games
        bool anyRunning = boardWindowList_.isAnyRunning();
        if (state_ == State::Starting && anyRunning) {
            state_ = State::Running;
        }
        if (state_ == State::Running && !anyRunning) {
            // Tournament just stopped - check if it finished naturally
            if (isFinished()) {
                auto sprtResult = sprtManager_->computeSprt();
                SnackbarManager::instance().showSuccess("SPRT tournament finished:\n" + sprtResult.info);
            }
            state_ = State::Stopped;
        }
        if (state_ == State::GracefulStopping && !anyRunning) {
            state_ = State::Stopped;
        }

    }
}

void SprtTournamentData::stopPool(bool graceful) {
    // Must be called before deactivating the control so that the pool is informed about 
    // setting concurrency to zero. 
    imguiConcurrency_->update(0);
    // Prevents that concurrency control tells the pool to start new tasks when calculations are stopped
    imguiConcurrency_->setActive(false);

    auto oldState = state_;
    state_ = graceful ? State::GracefulStopping : State::Stopped;
    if (!graceful) {
        // If we are not graceful, we stop all immediately
        poolAccess_->stopAll();
    }

    // Also stop Monte Carlo test if running
    bool wasMonteCarloRunning = isMonteCarloTestRunning();
    stopMonteCarloTest();

    if (oldState == State::Stopped && !wasMonteCarloRunning) {
        SnackbarManager::instance().showNote("SPRT tournament is not running.");
        return;
    }
    if (oldState == State::GracefulStopping && graceful && !wasMonteCarloRunning) {
        SnackbarManager::instance().showNote("SPRT tournament is already stopping gracefully.", 
            false, "sprt-tournament");
        return;
    }
   
    if (wasMonteCarloRunning) {
        SnackbarManager::instance().showSuccess("Monte Carlo test stopped", false, "sprt-tournament");
    } else {
        SnackbarManager::instance().showSuccess(
            graceful ? 
                "SPRT tournament stopped.\nFinishing ongoing games." : 
                "SPRT tournament stopped",
            false, "sprt-tournament"
        );
    }
}

void SprtTournamentData::clear() {
    if (!hasResults()) {
        SnackbarManager::instance().showNote("Nothing to clear.", false, "sprt-tournament");
        return;
    }
    std::string message = isRunning() ?
        "SPRT tournament stopped.\nAll SPRT tournament results have been cleared." :
        "All SPRT tournament results have been cleared.";
    imguiConcurrency_->setActive(false);
    state_ = State::Stopped;
    poolAccess_->clearAll();
    sprtManager_ = std::make_unique<SprtManager>();
    
    montecarloTable_.clear();
    
    SnackbarManager::instance().showSuccess(message, false, "sprt-tournament");
}

void SprtTournamentData::setPoolConcurrency(uint32_t count, bool nice, bool direct) {
    if (!isRunning()) {
        return;
    }
    imguiConcurrency_->setNiceStop(nice);
    imguiConcurrency_->update(count, direct);
}

uint32_t SprtTournamentData::getExternalConcurrency() const {
    return imguiConcurrency_->getExternalConcurrency();
}

void SprtTournamentData::setExternalConcurrency(uint32_t count) {
    imguiConcurrency_->setExternalConcurrency(count);
}

bool SprtTournamentData::hasResults() const {
    return (sprtManager_ != nullptr && sprtManager_->hasResults());
}

bool SprtTournamentData::isAnyRunning() const {
    return isRunning() || isMonteCarloTestRunning();
}

bool SprtTournamentData::isFinished() const {
    if (!sprtManager_) {
        return false;
    }
    return sprtManager_->isFinished();
}

void SprtTournamentData::activateBoardView([[maybe_unused]] size_t gameIndex) {
    // gameIndex is kept for compatibility with tournament activateBoardView interface
    // Might be required later
    if (!sprtManager_) {
        return;
    }
  
    boardWindowList_.setActiveWindowId("SPRT");
}

void SprtTournamentData::populateResultTable() {
    resultTable_.clear();

    if (!sprtManager_) {
        return;
    }

    auto duelResult = sprtManager_->getDuelResult();

    std::vector<std::string> row;
    row.push_back(duelResult.getEngineA());
    row.push_back(duelResult.getEngineB());
    row.push_back(std::format("{:.1f}%", duelResult.engineARate() * 100.0));
    row.push_back(std::to_string(duelResult.total()));
    resultTable_.push(row);
}

void SprtTournamentData::drawResultTable(const ImVec2& size) {
    auto duelResult = sprtManager_->getDuelResult();
    if (duelResult.total() == 0) {
        return;
    }
    resultTable_.draw(size, false);
}

void SprtTournamentData::populateSprtTable() {
    sprtTable_.clear();

    if (!sprtManager_) {
        return;
    }

    SprtResult sprtResult = sprtManager_->computeSprt();

    std::vector<std::string> row;
    row.push_back(sprtResult.engineA);
    row.push_back(sprtResult.engineB);
    row.push_back(sprtResult.info);
    sprtTable_.push(row);
}

void SprtTournamentData::drawSprtTable(const ImVec2& size) {
    auto duelResult = sprtManager_->getDuelResult();
    if (duelResult.total() == 0) {
        return;
    }
    sprtTable_.draw(size, false);
}

void SprtTournamentData::populateCausesTable() {
    if (!sprtManager_) {
        return;
    }

    auto duelResult = sprtManager_->getDuelResult();
    std::vector<EngineDuelResult> duelResults;
    
    // Add result from perspective of engine A
    duelResults.push_back(duelResult);
    
    // Add result from perspective of engine B (switched sides)
    duelResults.push_back(duelResult.switchedSides());
    
    causesTable_.populate(duelResults);
}

void SprtTournamentData::populateMonteCarloTable() {
    if (!sprtManager_) {
        return;
    }

    montecarloTable_.clear();
    
    sprtManager_->withMonteCarloResult([this](const QaplaTester::MonteCarloResult& result) {
        for (const auto& row : result.rows) {
            montecarloTable_.push(std::vector<std::string>{
                std::to_string(row.eloDifference),
                std::format("{:.1f}", row.noDecisionPercent),
                std::format("{:.1f}", row.h0AcceptedPercent),
                std::format("{:.1f}", row.h1AcceptedPercent),
                std::format("{:.1f}", row.avgGames)
            });
        }
    });
}

void SprtTournamentData::drawCauseTable(const ImVec2& size) {
    if (causesTable_.size() == 0) {
        return;
    }
    causesTable_.draw(size);
}

void SprtTournamentData::drawMonteCarloTable(const ImVec2& size) {
    if (montecarloTable_.size() == 0) {
        return;
    }
    montecarloTable_.draw(size);
}

void SprtTournamentData::saveTournament(const std::string& filename) {
    if (filename.empty()) {
        SnackbarManager::instance().showError("No filename specified for saving SPRT tournament.",
            false, "sprt-tournament");
        return;
    }

    try {
        std::ofstream out(filename, std::ios::trunc);
        if (!out) {
            SnackbarManager::instance().showError("Failed to open file for writing: " + filename,
                false, "sprt-tournament");
            return;
        }

        auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();

        // Save each section type (all with id "sprt-tournament")
        for (const auto& sectionName : sectionNames) {
            auto sections = configData.getSectionList(sectionName, "sprt-tournament");
            if (sections && !sections->empty()) {
                for (const auto& section : *sections) {
                    QaplaHelpers::IniFile::saveSection(out, section);
                }
            }
        }

        out.close();
        if (!out) {
            SnackbarManager::instance().showError("Error while writing to file: " + filename,
                false, "sprt-tournament");
            return;
        }

        SnackbarManager::instance().showSuccess("SPRT tournament saved to: " + filename,
            false, "sprt-tournament");
    }
    catch (const std::exception& e) {
        SnackbarManager::instance().showError("Failed to save SPRT tournament: " + std::string(e.what()),
            false, "sprt-tournament");
    }
}

void SprtTournamentData::loadTournament(const std::string& filename) {
    if (filename.empty()) {
        SnackbarManager::instance().showError("No filename specified for loading SPRT tournament.",
            false, "sprt-tournament");
        return;
    }

    try {
        std::ifstream in(filename);
        if (!in) {
            SnackbarManager::instance().showError("Failed to open file for reading: " + filename,
                false, "sprt-tournament");
            return;
        }

        // Load all sections from the file
        QaplaHelpers::ConfigData configData;
        configData.load(in);
        in.close();

        // Transfer sections to the global configuration
        for (const auto& sectionName : sectionNames) {
            auto sections = configData.getSectionList(sectionName, "sprt-tournament");
            if (sections && !sections->empty()) {
                QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
                    sectionName, "sprt-tournament", *sections);
            }
        }

        // Reload configuration from the updated singleton
        loadEngineSelectionConfig();
        tournamentOpening_->loadConfiguration();
        tournamentPgn_->loadConfiguration();
        loadSprtConfig();
        loadGlobalSettingsConfig();

        // Create tournament based on the configuration and load the tournament data
        loadTournament();

        SnackbarManager::instance().showSuccess("SPRT tournament loaded from: " + filename,
            false, "sprt-tournament");
    }
    catch (const std::exception& e) {
        SnackbarManager::instance().showError("Failed to load SPRT tournament: " + std::string(e.what()),
            false, "sprt-tournament");
    }
}

bool SprtTournamentData::runMonteCarloTest() {
    if (!sprtManager_) {
        SnackbarManager::instance().showError("Internal error, SPRT manager not initialized",
            false, "sprt-tournament");
        return false;
    }

    SnackbarManager::instance().showNote("Monte Carlo test started.");
    bool started = sprtManager_->runMonteCarloTest(*sprtConfig_);
    
    if (!started) {
        SnackbarManager::instance().showNote("Monte Carlo test is already running.",
            false, "sprt-tournament");
    }
    
    return started;
}

bool SprtTournamentData::isMonteCarloTestRunning() const {
    return sprtManager_ && sprtManager_->isMonteCarloTestRunning();
}

void SprtTournamentData::stopMonteCarloTest() {
    if (sprtManager_) {
        sprtManager_->stopMonteCarloTest();
    }
}
