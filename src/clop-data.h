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

#pragma once

#include "callback-manager.h"
#include "game-manager-pool-access.h"
#include "imgui-table.h"
#include "viewer-board-window-list.h"

#include <clop/clop-optimizer.h>
#include <clop/clop-types.h>
#include <engine-handling/engine-config.h>

#include <memory>
#include <string>
#include <vector>

/**
 * @file
 * @brief CLOP parameter tuning, driven from outside the GUI and watched inside it.
 *
 * The one activity with no tab of its own. It is set up and started over the remote control (see
 * QaplaLlm::RemoteControlServer), which is also where its numbers are read back -- so what would
 * be a configuration tab for the tournament, the SPRT test or the EPD analysis simply does not
 * exist here.
 *
 * What does exist is everything needed to *watch* it, because that is the whole point of running
 * it in the GUI rather than in qapla-engine-tester: the games appear on the boards while it runs.
 * That works without a tab because the board windows are not owned by one -- every
 * ViewerBoardWindowList registers itself and is drawn by ViewerBoardWindowList::drawAllTabs(), so
 * holding one here is all it takes.
 *
 * Deliberately not persisted, unlike the other three. A CLOP setup is one remote call and is
 * rebuilt in a moment; storing it would mean an ini section, a settings group and a load path for
 * a configuration nobody can edit in the window anyway.
 */

namespace QaplaWindows {

/**
 * @brief The CLOP run: its configuration, its optimizer, and the boards showing its games.
 */
class ClopData {
public:
    /** @brief How far along a CLOP run is. Mirrors the other activities' state machines. */
    enum class State { Idle, Starting, Running, Stopping, Stopped };

    [[nodiscard]] static ClopData& instance() {
        static ClopData data;
        return data;
    }

    ClopData(const ClopData&) = delete;
    ClopData& operator=(const ClopData&) = delete;

    /**
     * @brief The engine whose parameters are being tuned, by catalog name.
     *
     * Carries the gauntlet flag when handed to the optimizer, which is how
     * Clop::resolveOptimizingEngineIndex() tells it from its opponents -- the same convention the
     * SPRT test uses for its challenger.
     */
    void setEngines(const QaplaTester::EngineConfig& optimized,
        const std::vector<QaplaTester::EngineConfig>& opponents);

    [[nodiscard]] const std::string& optimizedEngineName() const { return optimizedName_; }

    /**
     * @brief Sets one time control on every participating engine.
     *
     * CLOP has no time control of its own -- CLOPConfig carries none -- so the games are played
     * at whatever each EngineConfig says. Setting them together is the only sane offer to make:
     * a tuning run in which the two sides think for different lengths measures the clock, not the
     * parameter. Returns false if the text is not a usable time control.
     */
    [[nodiscard]] bool setTimeControl(const std::string& timeControl);

    /** @brief The time control the engines share, or "" if they disagree or have none. */
    [[nodiscard]] std::string timeControl() const;
    [[nodiscard]] const std::vector<std::string>& opponentNames() const { return opponentNames_; }

    [[nodiscard]] QaplaTester::CLOPConfig& config() { return config_; }
    [[nodiscard]] const QaplaTester::CLOPConfig& config() const { return config_; }

    [[nodiscard]] unsigned int concurrency() const { return concurrency_; }
    void setConcurrency(unsigned int concurrency);

    /**
     * @brief Whether everything a run needs is set: an engine, at least one opponent, at least
     * one parameter, an openings file that exists, and a time control on every engine.
     *
     * The time control belongs in this list because nothing else checks it: CLOP schedules its
     * first games and only then does createGoLimits() find there is no clock to run, throwing on
     * a worker thread. Reporting "ready to start" without it is how a misconfiguration turned
     * into a terminated application.
     */
    [[nodiscard]] bool isReadyToStart() const;

    /**
     * @brief Builds the optimizer from the current configuration and schedules it.
     * @return An empty string on success, or why it could not start.
     */
    [[nodiscard]] std::string start();

    /** @brief Requests a stop. Games already running play out; nothing new is scheduled. */
    void stop();

    /** @brief Drops the optimizer and its samples, stopping the run first if one is going. */
    void clear();

    [[nodiscard]] State state() const { return state_; }
    [[nodiscard]] bool isRunning() const {
        return state_ == State::Starting || state_ == State::Running || state_ == State::Stopping;
    }

    /** @brief Whether the run reached the sample count it was configured for. */
    [[nodiscard]] bool isFinished() const;

    [[nodiscard]] std::size_t completedSamples() const;

    /** @brief The current best estimate, one entry per configured parameter. */
    [[nodiscard]] std::vector<std::pair<std::string, double>> estimatedParameters() const;

    /**
     * @brief Draws the live status and indicator tables.
     *
     * The pair the CLI writes to its report every outcomeInterval samples, except that here they
     * are redrawn every frame instead -- there is no interval to pick when the reader is a person
     * looking at a window, and a table that only moves every tenth sample looks stuck.
     *
     * Safe to call when nothing has run: it draws a line saying so rather than an empty frame.
     */
    void drawTables();

    /** @brief Whether there is anything for drawTables() to show. */
    [[nodiscard]] bool hasTables() const { return optimizer_ != nullptr; }

private:
    ClopData();
    ~ClopData();

    /** @brief Called once per frame: feeds the boards and advances the state machine. */
    void pollData();

    void populateResultTable();
    void populateIndicatorTable();

    /** @brief Fills one ImGuiTable from a TableData, taking its columns from the headers. */
    static void fill(ImGuiTable& target, const QaplaTester::TableData& source);

    std::unique_ptr<QaplaTester::CLOPOptimizer> optimizer_;
    QaplaTester::CLOPConfig config_{};

    std::string optimizedName_;
    std::vector<std::string> opponentNames_;
    std::vector<QaplaTester::EngineConfig> engines_;

    unsigned int concurrency_ = 1;
    State state_ = State::Idle;

    GameManagerPoolAccess poolAccess_;
    ViewerBoardWindowList boardWindowList_;
    ImGuiTable resultTable_;
    ImGuiTable indicatorTable_;

    std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
};

} // namespace QaplaWindows
