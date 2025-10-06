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

#include "engine-test-window.h"
#include "imgui-button.h"
#include "imgui-controls.h"
#include "snackbar.h"
#include "configuration.h"

#include <imgui.h>

#include <string>
#include <format>

using namespace QaplaWindows;

// UI Layout Constants
constexpr float standardIndent = 10.0F;

EngineTestWindow::EngineTestWindow()
    : engineSelect_(std::make_unique<ImGuiEngineSelect>())
{
    setEngineConfiguration();
    ImGuiEngineSelect::Options options;
    options.allowGauntletEdit = false;
    options.allowPonderEdit = false;
    options.allowTimeControlEdit = false;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = false;
    options.allowMultipleSelection = false;
    options.allowEngineOptionsEdit = false;
    engineSelect_->setOptions(options);
}

EngineTestWindow::~EngineTestWindow() = default;

void EngineTestWindow::setEngineConfigurationCallback(ImGuiEngineSelect::ConfigurationChangedCallback callback) {
    engineSelect_->setConfigurationChangedCallback(std::move(callback));
}

void EngineTestWindow::setEngineConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
            getConfigData().getSectionList("engineselection", "enginetest").value_or(std::vector<QaplaHelpers::IniFile::Section>{});
    engineSelect_->setId("enginetest");
    engineSelect_->setEngineConfiguration(sections);
}

std::vector<EngineConfig> EngineTestWindow::getSelectedEngineConfigurations() const
{
    std::vector<EngineConfig> selectedConfigs;
    const auto& configurations = engineSelect_->getEngineConfigurations();
    
    for (const auto& config : configurations) {
        if (config.selected) {
            selectedConfigs.push_back(config.config);
        }
    }
    
    return selectedConfigs;
}

static std::string getButtonText(const std::string& button, EngineTests::State testState) {
    if (button == "Run/Stop")
    {
        if (testState == EngineTests::State::Running) {
            return "Stop";
        }
        return "Run";
    } 
    return button;
}

static QaplaButton::ButtonState getButtonState(const std::string& button, EngineTests::State testState) {
    if (button == "Run/Stop")
    {
        if (testState == EngineTests::State::Running) {
            return QaplaButton::ButtonState::Active;
        }
        
        if (!EngineTests::instance().mayRun(false)) {
            return QaplaButton::ButtonState::Disabled;
        }
    } 
    if (button == "Clear")
    {
        if (!EngineTests::instance().mayClear(false)) {
            return QaplaButton::ButtonState::Disabled;
        }
    }
    return QaplaButton::ButtonState::Normal;
}

static void drawButtonIcon(const std::string& button, EngineTests::State testState, 
    QaplaButton::ButtonState buttonState, ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
    if (button == "Run/Stop") {
        if (testState == EngineTests::State::Running) {
            QaplaButton::drawStop(drawList, topLeft, size, buttonState);
        } else {
            QaplaButton::drawPlay(drawList, topLeft, size, buttonState);
        }
    } else if (button == "Clear") {
        QaplaButton::drawClear(drawList, topLeft, size, buttonState);
    }
}

static void handleButtonClick(const std::string& button, EngineTests::State testState, 
    const std::vector<EngineConfig>& selectedEngines) {
    try {
        if (button == "Run/Stop" && testState != EngineTests::State::Running) {
            // Start tests
            if (selectedEngines.empty()) {
                SnackbarManager::instance().showError("Please select at least one engine");
            } else {
                EngineTests::instance().runTests(selectedEngines);
            }
        } else if (button == "Run/Stop" && testState == EngineTests::State::Running) {
            // Stop tests
            EngineTests::instance().stop();
        } else if (button == "Clear") {
            // Clear results
            EngineTests::instance().clear();
        }
    } catch (const std::exception& e) {
        SnackbarManager::instance().showError(std::string("Error: ") + e.what());
    }
}

void EngineTestWindow::drawButtons()
{
    constexpr float space = 3.0F;
    constexpr float paddingTop = 5.0F;
    constexpr float paddingBottom = 8.0F;
    constexpr float paddingLeft = 20.0F;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = {25.0F, 25.0F};
    // We use "Analyze" to calculate the width of the button to keep the layout consistent
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + paddingLeft, boardPos.y + paddingTop);
    
    auto testState = EngineTests::instance().getState();
    auto selectedEngines = getSelectedEngineConfigurations();
    
    for (const std::string button : {"Run/Stop", "Clear"})
    {
        ImGui::SetCursorScreenPos(pos);
        
        auto buttonText = getButtonText(button, testState);
        auto buttonState = getButtonState(button, testState);
       
        if (QaplaButton::drawIconButton(button, buttonText, buttonSize, buttonState, 
            [&button, buttonState, testState](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
            {
                drawButtonIcon(button, testState, buttonState, drawList, topLeft, size);
            }
        ))
        {
            handleButtonClick(button, testState, selectedEngines);
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + paddingTop + paddingBottom));
}

void EngineTestWindow::drawInput()
{
    if (ImGui::CollapsingHeader("Engines", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("engineSettings");
        ImGui::Indent(standardIndent);
        engineSelect_->draw();
        ImGui::Unindent(standardIndent);
        ImGui::PopID();
    }
}

static void drawCheckbox(const char* label, bool& value, const char* tooltip) {
    if (ImGui::Checkbox(label, &value)) {
        EngineTests::instance().updateConfiguration();
    }
    if (ImGui::IsItemHovered() && tooltip != nullptr) {
        ImGui::SetTooltip("%s", tooltip);
    }
}

static void drawTests()
{
    constexpr uint32_t maxGames = 10000;
    constexpr uint32_t maxConcurrency = 32;
    
    if (ImGui::CollapsingHeader("Tests", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(standardIndent);
        
        bool modified = false;
        auto& testSelection = EngineTests::instance().getTestSelection();
        
        drawCheckbox("Start/Stop Test", testSelection.testStartStop, "Test basic engine start and stop functionality");
        drawCheckbox("Hash Table Memory Test", testSelection.testHashTableMemory, "Test that memory usage shrinks when reducing Hash option");
        drawCheckbox("Lowercase Option Test", testSelection.testLowerCaseOption, "Test that engine accepts lowercase option names");
        drawCheckbox("Engine Options Test", testSelection.testEngineOptions, "Test all engine options with edge case values");
        drawCheckbox("Analyze Test", testSelection.testAnalyze, "Test that engine reacts correctly to stop command during analysis");
        drawCheckbox("Immediate Stop Test", testSelection.testImmediateStop, "Test that engine handles immediate stop command correctly");
        drawCheckbox("Infinite Analyze Test", testSelection.testInfiniteAnalyze, "Test that engine correctly handles infinite analysis mode");
        drawCheckbox("Go Limits Test", testSelection.testGoLimits, "Test various time limits, depth limits, and node limits");
        drawCheckbox("EP from FEN Test", testSelection.testEpFromFen, "Test en passant handling from FEN position");
        drawCheckbox("Compute Game Test", testSelection.testComputeGame, "Test engine playing a complete game against itself");
        drawCheckbox("Ponder Test", testSelection.testPonder, "Test pondering functionality (ponder hit and miss)");
        drawCheckbox("EPD Test", testSelection.testEpd, "Test engine finding correct moves for standardized positions");
        drawCheckbox("Multiple Games Test", testSelection.testMultipleGames, "Test playing multiple games in parallel");
        
        // Show options for Multiple Games Test if enabled
        if (testSelection.testMultipleGames) {
            ImGui::Indent(standardIndent);

            modified |= QaplaWindows::ImGuiControls::inputInt<uint32_t>("Number of Games", testSelection.numGames, 1, maxGames);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Total number of games to play");
            }

            modified |= QaplaWindows::ImGuiControls::inputInt<uint32_t>("Concurrency", testSelection.concurrency, 1, maxConcurrency);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Number of games to play in parallel");
            }
            
            ImGui::Unindent(standardIndent);
        }
        
        if (modified) {
            EngineTests::instance().updateConfiguration();
        }
        
        ImGui::Unindent(standardIndent);
    }
}

void EngineTestWindow::drawReportTables()
{
    // Get the selected engine configurations
    auto selectedEngines = getSelectedEngineConfigurations();
    constexpr float TableHeight = 600.0F;
    
    if (selectedEngines.empty()) {
        return;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Draw a collapsing header for each selected engine
    for (const auto& engineConfig : selectedEngines) {
        std::string headerLabel = engineConfig.getName() + " Report";
        
        if (ImGui::CollapsingHeader(headerLabel.c_str())) {
            
            // Create and draw the report table for this engine
            auto reportTable = EngineTests::instance().createReportTable(engineConfig.getName());
            
            if (reportTable) {
                ImVec2 tableSize = ImGui::GetContentRegionAvail();
                reportTable->draw(ImVec2(tableSize.x, TableHeight), true);
            } else {
                ImGui::TextDisabled("No report data available");
            }
            
        }
    }
}

void EngineTestWindow::draw()
{
    constexpr float rightBorder = 5.0F;
    constexpr float minTableSpace = 50.0F;
    drawButtons();

    ImGui::Indent(standardIndent);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("InputArea", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None);

    drawInput();
    drawTests();
    drawReportTables();
    
    // Draw results table
    ImVec2 tableSize = ImGui::GetContentRegionAvail();
    if (tableSize.y > minTableSpace) {  // Only draw if there's enough space
        ImGui::Spacing();
        ImGui::Text("Test Results:");
        ImGui::Spacing();
        EngineTests::instance().drawTable(ImVec2(tableSize.x, 0.0F));
    }
    
    ImGui::EndChild();
    ImGui::Unindent(standardIndent);
}
