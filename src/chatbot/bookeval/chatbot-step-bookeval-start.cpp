/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "chatbot-step-bookeval-start.h"
#include "book-eval-data.h"
#include "imgui-controls.h"

#include "ui/qapla-ui.h"

#include <format>

namespace QaplaWindows::ChatBot {

namespace {
    constexpr uint32_t MIN_CONCURRENCY = 1;
    constexpr uint32_t MAX_CONCURRENCY = 64;
}

std::string ChatbotStepBookEvalStart::draw() {
    if (finished_) {
        return "";
    }
    auto& data = BookEvalData::instance();
    if (data.isBusy()) {
        drawWhileRunning();
        return "";
    }
    if (started_) {
        // The run is over - a small book can be done before this step is drawn again.
        ImGuiControls::textWrapped(data.isComplete()
            ? std::format("The book evaluation is finished: all {} positions evaluated, written to {}.",
                data.getPositionCount(), data.config().outFile)
            : std::format("The book evaluation stopped with {} of {} positions evaluated. Start it "
                "again with the same books to continue.", data.getEvaluatedCount(), data.getPositionCount()));
        QaplaUi::spacing();
        if (ImGuiControls::textButton("Close")) {
            finished_ = true;
        }
        return "";
    }
    return drawBeforeStart();
}

std::string ChatbotStepBookEvalStart::drawBeforeStart() {
    auto& data = BookEvalData::instance();

    ImGuiControls::textWrapped(
        "Set how many engines search at the same time and start the run. You can change the number "
        "while it runs.");
    QaplaUi::spacing();
    auto concurrency = data.getExternalConcurrency();
    if (ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, MIN_CONCURRENCY, MAX_CONCURRENCY)) {
        data.setExternalConcurrency(concurrency);
        data.updateConfiguration();
    }
    ImGuiControls::hooverTooltip("Number of positions searched in parallel, each by an engine of its own.");

    QaplaUi::spacing();
    QaplaUi::separator();
    QaplaUi::spacing();

    QaplaUi::beginDisabled(!data.mayEvaluate());
    if (ImGuiControls::textButton("Start Evaluation")) {
        data.evaluate();
        started_ = true;
    }
    ImGuiControls::hooverTooltip("Search every position of the book that has no score yet.");
    QaplaUi::endDisabled();

    QaplaUi::sameLine();
    if (ImGuiControls::textButton("Cancel")) {
        finished_ = true;
        return "stop";
    }
    return "";
}

void ChatbotStepBookEvalStart::drawWhileRunning() {
    auto& data = BookEvalData::instance();
    const auto evaluated = data.getEvaluatedCount();
    const auto total = data.getPositionCount();

    ImGuiControls::textWrapped(std::format(
        "The book evaluation is running: {} of {} positions evaluated, {:.1f} positions per second.",
        evaluated, total, data.getPositionsPerSecond()));
    QaplaUi::spacing();
    QaplaUi::progressBar(total == 0 ? 0.0F : static_cast<float>(evaluated) / static_cast<float>(total),
        QaplaUi::Vec2(-10.0F, 20.0F));

    QaplaUi::spacing();
    auto concurrency = data.getExternalConcurrency();
    if (ImGuiControls::sliderInt<uint32_t>("Concurrency", concurrency, MIN_CONCURRENCY, MAX_CONCURRENCY)) {
        data.setExternalConcurrency(concurrency);
        data.setPoolConcurrency(concurrency, true, true);
        data.updateConfiguration();
    }
    ImGuiControls::hooverTooltip("Changes how many positions are searched at the same time, right away.");

    QaplaUi::spacing();
    QaplaUi::separator();
    QaplaUi::spacing();
    if (ImGuiControls::textButton("Stop")) {
        data.stop(false);
    }
    ImGuiControls::hooverTooltip("Stop the evaluation. The evaluated book keeps every score found so far, "
        "and starting again continues from there.");
}

} // namespace QaplaWindows::ChatBot
