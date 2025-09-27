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

#include "epd-window.h"
#include "imgui-table.h"
#include "imgui-button.h"
#include "snackbar.h"
#include "os-dialogs.h"
#include "imgui-controls.h"
#include "epd-data.h"
#include "configuration.h"

#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include "qapla-tester/string-helper.h"
#include "qapla-tester/engine-event.h"
#include "qapla-tester/game-manager-pool.h"

#include <imgui.h>

#include <sstream>
#include <string>
#include <format>
#include <memory>

using namespace QaplaWindows;

EpdWindow::EpdWindow()
    : engineSelect_(std::make_unique<ImGuiEngineSelect>())
{
    setEngineConfiguration();
    ImGuiEngineSelect::Options options;
    options.allowGauntletEdit = false;
    engineSelect_->setOptions(options);
}

EpdWindow::~EpdWindow() = default;

void EpdWindow::setEngineConfigurationCallback(ImGuiEngineSelect::ConfigurationChangedCallback callback) {
    engineSelect_->setConfigurationChangedCallback(callback);
}

void EpdWindow::setEngineConfiguration() {
    auto sections = QaplaConfiguration::Configuration::instance().
            getConfigData().getSectionList("engineselection", "epd").value_or({});
    engineSelect_->setId("epd");
    engineSelect_->setEngineConfiguration(sections);
}

static std::string getButtonText(const std::string& button, EpdData::State epdState) {
    if (button == "Run/Stop")
    {
        if (epdState == EpdData::State::Running) {
            return "Stop";
        } else {
            return EpdData::instance().configChanged() ? "Analyze" : "Continue";
        } 
    } 
    return button;
}

static QaplaButton::ButtonState getButtonState(const std::string& button, EpdData::State epdState) {
    if (button == "Run/Stop")
    {
        if (epdState == EpdData::State::Running) {
            return QaplaButton::ButtonState::Active;
        } 
        // Manual clear is now required before re-running analysis
        if (EpdData::instance().configChanged() && epdState == EpdData::State::Stopped) {
            return QaplaButton::ButtonState::Disabled;
        }
    } 
    if (button == "Grace")
    {
         if (epdState == EpdData::State::Stopping) {
            return QaplaButton::ButtonState::Active;
        } else if (epdState != EpdData::State::Running) {
            return QaplaButton::ButtonState::Disabled;
        } 
    } 
    if (button == "Clear")
    {
        if (epdState != EpdData::State::Stopped) {
            return QaplaButton::ButtonState::Disabled;
        }
    }
    return QaplaButton::ButtonState::Normal;
}

void EpdWindow::drawButtons()
{
    constexpr float space = 3.0f;
    constexpr float topOffset = 5.0f;
    constexpr float bottomOffset = 8.0f;
    constexpr float leftOffset = 20.0f;
    ImVec2 boardPos = ImGui::GetCursorScreenPos();

    constexpr ImVec2 buttonSize = {25.0f, 25.0f};
    const auto totalSize = QaplaButton::calcIconButtonTotalSize(buttonSize, "Analyze");
    auto pos = ImVec2(boardPos.x + leftOffset, boardPos.y + topOffset);
    for (const std::string button : {"Run/Stop", "Grace", "Clear"})
    {
        ImGui::SetCursorScreenPos(pos);
        
        auto buttonText = getButtonText(button, EpdData::instance().state);
        auto buttonState = getButtonState(button, EpdData::instance().state);
       
        if (QaplaButton::drawIconButton(button, buttonText, buttonSize, buttonState, 
            [&button, buttonState](ImDrawList *drawList, ImVec2 topLeft, ImVec2 size)
            {
                auto epdState = EpdData::instance().state;
                if (button == "Run/Stop")
                {
                    if (epdState == EpdData::State::Running) {
                        QaplaButton::drawStop(drawList, topLeft, size, buttonState);
                    } else {
                        QaplaButton::drawPlay(drawList, topLeft, size, buttonState);
                    }
                }
                if (button == "Grace")
                {
                     QaplaButton::drawGrace(drawList, topLeft, size, buttonState);
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
                auto epdState = EpdData::instance().state;
                if (button == "Run/Stop" && epdState != EpdData::State::Running)
                {
                    EpdData::instance().analyse();
                }
                else if (button == "Run/Stop" && epdState == EpdData::State::Running)
                {
                    EpdData::instance().stopPool(false);
                }
                else if (button == "Grace") {
                    EpdData::instance().stopPool(true);
                }
                else if (button == "Clear" && epdState == EpdData::State::Stopped)
                {
                    EpdData::instance().clear();
                }
            }
            catch (const std::exception &e)
            {
                SnackbarManager::instance().showError(std::string("Fehler: ") + e.what());
            }
        }
        pos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(boardPos.x, boardPos.y + totalSize.y + topOffset + bottomOffset));
}

void EpdWindow::drawInput()
{

    constexpr float inputWidth = 200.0f;
    constexpr int maxConcurrency = 32;
    constexpr int maxSeenPlies = 32;

    ImGui::Indent(10.0f);
    ImGui::SetNextItemWidth(inputWidth);
    bool modified = false;

    if (ImGuiControls::sliderInt<uint32_t>("Concurrency",
                                           EpdData::instance().config().concurrency, 1, maxConcurrency))
    {
        if (EpdData::instance().state == EpdData::State::Running) {
            GameManagerPool::getInstance().setConcurrency(EpdData::instance().config().concurrency, true, true);
        }
        modified = true;
    }

    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("Engines", ImGuiTreeNodeFlags_Selected)) {
        ImGui::PushID("engineSettings");
        ImGui::Indent(10.0f);
        engineSelect_->draw();
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }
    
    if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_Selected))
    {
        ImGui::Indent(10.0f);
        std::string filePath = EpdData::instance().config().filepath;
        ImGui::SetNextItemWidth(inputWidth);
        modified |= ImGuiControls::inputInt<uint32_t>("Seen plies",
                                        EpdData::instance().config().seenPlies, 1, maxSeenPlies);

        ImGui::SetNextItemWidth(inputWidth);
        modified |= ImGuiControls::inputInt<uint64_t>("Max time (s)",
                                        EpdData::instance().config().maxTimeInS, 1, 3600 * 24 * 365, 1, 100);

        ImGui::SetNextItemWidth(inputWidth);
        modified |= ImGuiControls::inputInt<uint64_t>("Min time (s)",
                                        EpdData::instance().config().minTimeInS, 1, 3600 * 24 * 365, 1, 100);

        ImGui::Spacing();
        modified |= ImGuiControls::existingFileInput("Epd or RAW position file:",
                                EpdData::instance().config().filepath, inputWidth * 2.0f);
        ImGui::Spacing();
        ImGui::Unindent(10.0f);

    }
    if (modified) {
        EpdData::instance().updateConfiguration();
    }
    ImGui::Unindent(10.0f);
}

void EpdWindow::drawProgress()
{
    auto total = EpdData::instance().totalTests;
    auto remaining = EpdData::instance().remainingTests;
    if (total == 0 || remaining == 0) {
        return;
    }
    ImGui::Indent(10.0f);
    size_t testFinished = total - remaining;
    float progress = static_cast<float>(testFinished) / static_cast<float>(total);
    ImGui::ProgressBar(progress, ImVec2(-10.0f, 20.0f), std::to_string(testFinished).c_str());
    ImGui::Unindent(10.0f);
}

void EpdWindow::draw()
{
    drawButtons();
    drawInput();
    drawProgress();
    ImGui::Indent(10.0f);
    ImVec2 size = ImGui::GetContentRegionAvail();
    auto clickedRow = EpdData::instance().drawTable(size);
    ImGui::Unindent(10.0f);
    EpdData::instance().setSelectedIndex(clickedRow);
}
