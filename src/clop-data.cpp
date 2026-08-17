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

#include "clop-data.h"

#include <game-manager/game-manager-pool.h>

#include <filesystem>
#include <format>
#include <system_error>

namespace QaplaWindows {

namespace {
    /** @brief One TableCell as text, whichever of its three types it holds. */
    std::string cellText(const QaplaTester::TableCell& cell) {
        return std::visit(
            [](const auto& value) -> std::string {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, std::string>) {
                    return value;
                } else {
                    return std::format("{}", value);
                }
            },
            cell.value);
    }
} // namespace

ClopData::ClopData()
    : boardWindowList_("CLOP") {
    // Its own pool, like every other activity: one pool per activity is what lets a CLOP run and
    // a tournament exist side by side without either one's concurrency reaching the other.
    poolAccess_ = GameManagerPoolAccess(std::make_shared<QaplaTester::GameManagerPool>());
    boardWindowList_.setPoolAccess(poolAccess_);

    pollCallbackHandle_ = StaticCallbacks::poll().registerCallback([this]() { this->pollData(); });
}

ClopData::~ClopData() = default;

void ClopData::setEngines(const QaplaTester::EngineConfig& optimized,
    const std::vector<QaplaTester::EngineConfig>& opponents) {
    engines_.clear();
    opponentNames_.clear();

    // The gauntlet flag carries the role, exactly as it does for the SPRT test's challenger:
    // Clop::resolveOptimizingEngineIndex() reads it back to find the engine under tuning, and
    // rejects a list with more than one. Both roles are written every time, because an engine
    // that just swapped roles would otherwise keep the flag it had.
    auto tuned = optimized;
    tuned.setGauntlet(true);
    engines_.push_back(tuned);
    optimizedName_ = tuned.getName();

    for (const auto& opponent : opponents) {
        auto config = opponent;
        config.setGauntlet(false);
        engines_.push_back(config);
        opponentNames_.push_back(config.getName());
    }
}

void ClopData::setConcurrency(unsigned int concurrency) {
    concurrency_ = concurrency;
    // Takes effect on a run that is already going, which is the one setting that does -- see
    // Actions::concurrencySentence() for why that is worth saying out loud to a caller.
    if (isRunning()) {
        poolAccess_->setConcurrency(concurrency_, true);
    }
}

bool ClopData::setTimeControl(const std::string& timeControl) {
    // Applied to a copy first: setTimeControl() throws on text it cannot parse, and half the
    // engines carrying the new clock while the rest keep the old one is a worse state than the
    // one before the call.
    auto updated = engines_;
    for (auto& engine : updated) {
        try {
            engine.setTimeControl(timeControl);
        } catch (const std::exception&) {
            return false;
        }
        if (!engine.getTimeControl().isValid()) {
            return false;
        }
    }
    engines_ = std::move(updated);
    return true;
}

std::string ClopData::timeControl() const {
    if (engines_.empty()) {
        return "";
    }
    const auto first = engines_.front().getTimeControl().toPgnTimeControlString();
    for (const auto& engine : engines_) {
        if (engine.getTimeControl().toPgnTimeControlString() != first) {
            return "";
        }
    }
    return first;
}

bool ClopData::isReadyToStart() const {
    if (optimizedName_.empty() || opponentNames_.empty() || config_.parameters.empty()) {
        return false;
    }
    // Every engine, not just one: createGoLimits() checks both sides of every game and throws on
    // a worker thread if either has no clock, which ends the process rather than the run.
    for (const auto& engine : engines_) {
        if (!engine.getTimeControl().isValid()) {
            return false;
        }
    }
    std::error_code error;
    return !config_.openingsFile.empty() && std::filesystem::exists(config_.openingsFile, error);
}

std::string ClopData::start() {
    if (isRunning()) {
        return "A CLOP run is already going.";
    }
    if (!isReadyToStart()) {
        return "The CLOP run is not fully configured yet.";
    }

    // A fresh optimizer per run: scheduleCLOP() refuses a second scheduling on the same object,
    // and the samples of a previous run have no meaning for a new parameter set anyway.
    optimizer_ = std::make_unique<QaplaTester::CLOPOptimizer>();
    try {
        optimizer_->createCLOP(engines_, config_);
        optimizer_->scheduleCLOP(concurrency_, *poolAccess_);
    } catch (const std::exception& ex) {
        optimizer_.reset();
        state_ = State::Idle;
        return ex.what();
    }

    state_ = State::Starting;
    return "";
}

void ClopData::stop() {
    if (!optimizer_) {
        return;
    }
    state_ = State::Stopping;

    // The pool first, then the optimizer. CLOPOptimizer::stop() joins its scheduler thread, and
    // that thread is waiting on game results -- stopping it before the games would block the UI
    // thread for as long as the longest game still had to run.
    poolAccess_->stopAll();
    optimizer_->stop();
    state_ = State::Stopped;
}

void ClopData::clear() {
    if (isRunning()) {
        stop();
    }
    poolAccess_->clearAll();
    optimizer_.reset();
    resultTable_.clear();
    state_ = State::Idle;
}

bool ClopData::isFinished() const {
    // Inferred rather than asked: the optimizer keeps its own finished flag private, and reaching
    // the configured sample count is what that flag means.
    return optimizer_ != nullptr && config_.samples > 0 &&
        optimizer_->getCompletedSamples() >= config_.samples;
}

std::size_t ClopData::completedSamples() const {
    return optimizer_ ? optimizer_->getCompletedSamples() : 0U;
}

std::vector<std::pair<std::string, double>> ClopData::estimatedParameters() const {
    std::vector<std::pair<std::string, double>> estimate;
    if (!optimizer_) {
        return estimate;
    }
    const auto values = optimizer_->getEstimatedParameters();
    for (std::size_t i = 0; i < config_.parameters.size() && i < values.size(); ++i) {
        estimate.emplace_back(config_.parameters[i].name, values[i]);
    }
    return estimate;
}

void ClopData::populateResultTable() {
    resultTable_.clear();
    if (!optimizer_) {
        return;
    }

    const auto table = optimizer_->getStatusTable();
    if (table.headers.empty()) {
        return;
    }

    // Columns come from the optimizer rather than being declared here: its status table changes
    // shape with the number of parameters being tuned, so a fixed column list would either be
    // wrong or would have to be rebuilt on every configuration change anyway.
    resultTable_.resizeColumns(table.headers.size());
    for (std::size_t column = 0; column < table.headers.size(); ++column) {
        resultTable_.setColumnHead(column, ImGuiTable::ColumnDef{.name = table.headers[column]});
    }

    for (const auto& row : table.body) {
        std::vector<std::string> cells;
        cells.reserve(row.size());
        for (const auto& cell : row) {
            cells.push_back(cellText(cell));
        }
        resultTable_.push(cells);
    }
}

std::string ClopData::resultsAsText() {
    populateResultTable();
    return resultTable_.toText();
}

void ClopData::pollData() {
    if (state_ == State::Idle) {
        return;
    }

    boardWindowList_.populateViews();
    populateResultTable();

    const bool anyRunning = boardWindowList_.isAnyRunning();
    if (state_ == State::Starting && anyRunning) {
        state_ = State::Running;
    }
    if (state_ == State::Running && !anyRunning && isFinished()) {
        // Only on its own conclusion. A momentary gap between two samples also shows no running
        // game, which is why the sample count decides and not the boards.
        stop();
    }
}

} // namespace QaplaWindows
