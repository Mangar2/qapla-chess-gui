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
#include "snackbar.h"
#include "configuration.h"

#include <imgui.h>

#include <string>
#include <format>

using namespace QaplaWindows;

EngineTestWindow::EngineTestWindow()
    : engineSelect_(std::make_unique<ImGuiEngineSelect>())
    , isRunning_(false)
    , testStartStopSelected_(true)
{
    setEngineConfiguration();
    ImGuiEngineSelect::Options options;
    options.allowGauntletEdit = false;
    options.allowPonderEdit = false;
    options.allowTimeControlEdit = false;
    options.allowTraceLevelEdit = true;
    options.allowRestartOptionEdit = false;
    options.allowMultipleSelection = true;
    engineSelect_->setOptions(options);
}

EngineTestWindow::~EngineTestWindow() = default;

void EngineTestWindow::setEngineConfigurationCallback(ImGuiEngineSelect::ConfigurationChangedCallback callback) {
    engineSelect_->setConfigurationChangedCallback(callback);
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

static std::string getButtonText(const std::string& button, bool isRunning) {
    if (button == "Run/Stop")
    {
        return isRunning ? "Stop" : "Run";
    } 
    return button;
}

static QaplaButton::ButtonState getButtonState(const std::string& button, bool isRunning) {
    if (button == "Run/Stop")
    {
        return isRunning ? QaplaButton::ButtonState::Active : QaplaButton::ButtonState::Normal;
    } 
    if (button == "Clear")
    {
        if (isRunning) {
            return QaplaButton::ButtonState::Disabled;
        }
    }
    return QaplaButton::ButtonState::Normal;
}

void EngineTestWindow::drawButtons()
{
    constexpr float space = 3.0f;
    constexpr float paddingTop = 5.0f;
    constexpr float paddingBottom = 8.0f;
    constexpr float paddingLeft = 20.0f;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = {25.0f, 25.0f};
    // We use "Analyze" to calculate the width of the button to keep the layout consistent
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + paddingLeft, boardPos.y + paddingTop);
    
    for (const std::string button : {"Run/Stop", "Clear"})
    {
        ImGui::SetCursorScreenPos(pos);
        
        auto buttonText = getButtonText(button, isRunning_);
        auto buttonState = getButtonState(button, isRunning_);
       
        if (QaplaButton::drawIconButton(button, buttonText, buttonSize, buttonState, 
            [&button, buttonState, this](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
            {
                if (button == "Run/Stop")
                {
                    if (isRunning_) {
                        QaplaButton::drawStop(drawList, topLeft, size, buttonState);
                    } else {
                        QaplaButton::drawPlay(drawList, topLeft, size, buttonState);
                    }
                }
                if (button == "Clear")
                {
                    QaplaButton::drawClear(drawList, topLeft, size, buttonState);
                } 
            }
        ))
        {
            try
            {
                if (button == "Run/Stop" && !isRunning_)
                {
                    // Start tests
                    auto selectedEngines = getSelectedEngineConfigurations();
                    if (selectedEngines.empty()) {
                        SnackbarManager::instance().showError("Bitte mindestens eine Engine auswählen");
                        return;
                    }
                    
                    isRunning_ = true;
                    if (testStartStopSelected_) {
                        EngineTests::instance().testEngineStartStop(selectedEngines);
                    }
                    isRunning_ = false;
                }
                else if (button == "Run/Stop" && isRunning_)
                {
                    // Stop tests
                    isRunning_ = false;
                }
                else if (button == "Clear" && !isRunning_)
                {
                    // Clear results
                    // TODO: Implement clear functionality
                }
            }
            catch (const std::exception &e)
            {
                SnackbarManager::instance().showError(std::string("Fehler: ") + e.what());
                isRunning_ = false;
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + paddingTop + paddingBottom));
}

void EngineTestWindow::drawInput()
{
    if (ImGui::CollapsingHeader("Engines", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("engineSettings");
        ImGui::Indent(10.0f);
        engineSelect_->draw();
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }
}

void EngineTestWindow::drawTests()
{
    if (ImGui::CollapsingHeader("Tests", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);
        
        // Engine Start/Stop Test
        ImGui::Checkbox("Engine Start/Stop Test", &testStartStopSelected_);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Test that engines can be properly started and stopped");
        }
        
        // Future tests will be added here
        
        ImGui::Unindent(10.0f);
    }
}

void EngineTestWindow::draw()
{
    constexpr float rightBorder = 5.0f;
    drawButtons();

    ImGui::Indent(10.0f);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("InputArea", ImVec2(size.x - rightBorder, 0), ImGuiChildFlags_None);

    drawInput();
    drawTests();
    
    // TODO: Add test results display area
    
    ImGui::EndChild();
    ImGui::Unindent(10.0f);
}
