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

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/**
 * @file
 * @brief Vocabulary of the INTERNAL tool layer (src/llm/actions/).
 *
 * Everything in src/llm/actions/ is deliberately free of JSON, of tool names, and of anything
 * else the model ever sees: an action takes plain C++ arguments, does the GUI work, and reports
 * back as an ActionResult. That is what makes reshaping the model-facing API (renaming a field,
 * merging two tools into one, changing an enum's wording, moving a parameter elsewhere) a change
 * confined to src/llm/tools/ -- see src/llm/api/llm-tool-api.h for the mapper that joins the two.
 *
 * Rule of thumb for which layer a piece of behaviour belongs to: if changing it would change what
 * the model sees, it is external (src/llm/tools/); if it would change what the GUI does, it is
 * internal (here).
 */

namespace QaplaLlm::Actions {

/**
 * @brief How a running activity should be brought to a halt.
 *
 * Graceful lets whatever is already in progress finish while refusing to start anything new;
 * Abrupt aborts in-progress work immediately.
 */
enum class StopMode { Graceful, Abrupt };

/**
 * @brief The one run-state vocabulary every activity is reported in.
 *
 * The tournament, the SPRT test and the EPD analysis each keep their own state enum, and each
 * used to phrase those states in its own words -- three hand-written wordings that agreed on the
 * hard parts (never calling a gracefully stopping run "running") and drifted on the easy ones
 * ("stopping abruptly" vs "being aborted" for the same state). A caller has no way to tell an
 * intended difference from an accidental one, so the wording lives here once and every activity
 * maps its own states onto it. A fourth activity (CLOP) only has to supply that mapping.
 */
enum class RunState {
    Idle,
    Starting,
    Running,

    /** @brief Stop requested, in-progress work still playing out, nothing new started. */
    FinishingAfterGracefulStop,

    /** @brief Abort under way; in-progress work is being torn down. */
    Aborting
};

/**
 * @brief What one activity is called, in the three forms the shared sentences need.
 *
 * Declared per activity next to its state mapping, so the wording below stays free of any
 * activity-specific branching.
 */
struct ActivityNames {
    std::string_view withArticle; ///< "a tournament", "an SPRT test", "an EPD analysis"
    std::string_view bare;        ///< "tournament", "SPRT test", "EPD analysis"
    std::string_view workItems;   ///< What it processes: "games", "positions"
};

inline constexpr ActivityNames TOURNAMENT_NAMES{
    .withArticle = "a tournament", .bare = "tournament", .workItems = "games"};
inline constexpr ActivityNames SPRT_NAMES{
    .withArticle = "an SPRT test", .bare = "SPRT test", .workItems = "games"};
inline constexpr ActivityNames EPD_NAMES{
    .withArticle = "an EPD analysis", .bare = "EPD analysis", .workItems = "positions"};

/**
 * @brief The sentence an idle activity adds to its status when it could be started as it stands.
 *
 * It exists because "nothing is running" and "nothing is configured" are not the same fact, and
 * only the first was ever reported. A caller told the first had no way to tell a fully configured
 * activity from an empty one except by asking after each in turn -- and one that doesn't ask
 * concludes it has to set something up first. That is how "just start the SPRT test" turned into
 * listing the engine catalog and opening the install dialog, on a test that was ready all along.
 *
 * Deliberately one-sided: an activity that is NOT ready says nothing about it. Naming an obstacle
 * reads as an assignment -- "its results have to be cleared first" is one resolution out of
 * several, and the destructive one, so a caller that takes it at face value discards results
 * nobody asked it to discard. Silence is not a gap either: the status still lists the engines and
 * files themselves, and the moment readiness actually matters is when a start is attempted, where
 * the refusal names its own reason to a caller who has just asked for exactly that.
 */
[[nodiscard]] std::string readyToStartSentence();

/** @brief One full sentence naming the run state, for a status report. */
[[nodiscard]] std::string runStateSentence(RunState state, const ActivityNames& names);

/**
 * @brief The same state as a lowercase clause for the cross-activity summary ("a tournament is
 * running with concurrency 8"), or "" when idle -- an idle activity is dropped from that summary
 * rather than carrying an "is idle" wording that only reads well on its own.
 *
 * The summary is the one answer that says what is going on without listing any settings, so the
 * one setting that decides how fast it is going belongs in it. Only stated for the states where
 * it is in force; see the note in the implementation.
 */
[[nodiscard]] std::string runStatePhrase(
    RunState state, const ActivityNames& names, unsigned int concurrency);

/**
 * @brief How far a run's own configuration is frozen right now.
 *
 * Two phases rather than one because the advice differs: a running activity can still be stopped,
 * one that is already stopping only needs waiting out. Telling the second to "stop it first" is
 * what used to send callers round a stop/start loop that could never succeed.
 */
enum class RunLock { None, Running, Stopping };

/** @brief Maps a run state onto the lock it implies. */
[[nodiscard]] RunLock lockOf(RunState state);

/** @brief What one activity is doing, in the two facts an onlooker needs. */
struct ActivityProgress {
    RunState state = RunState::Idle;

    /**
     * @brief Whether it ran to its own conclusion, as opposed to merely not running.
     *
     * Separates "the SPRT reached a decision" from "someone stopped it" -- the same state, and
     * very different news for whoever was waiting on it (see QaplaLlm::ActivityWatch).
     */
    bool finished = false;
};

/**
 * @brief Why a settings or engine change is refused right now, or "" if it may go through.
 *
 * One message covers both because engine selection is a configuration field like any other; it is
 * frozen for exactly as long, and for the same reason (a run whose engines or settings changed
 * halfway would produce one result table measured under two different rules).
 */
[[nodiscard]] std::string settingsLockedSentence(RunLock lock, const ActivityNames& names);

/**
 * @brief The same fact as a note for a status report ("locked right now, only concurrency"), so a
 * caller learns it by asking rather than by trying and being refused. Returns "" when unlocked.
 */
[[nodiscard]] std::string settingsLockNote(RunLock lock);

/**
 * @brief Reports a concurrency change, in the one sentence a caller needs to close the matter.
 *
 * Concurrency is the only setting that takes effect on a run that is already going, so it gets a
 * sentence of its own rather than one "concurrency=15" item inside the full status, which read as
 * if the change had been ignored and provoked stop/start cycles to force it through.
 *
 * Two things that sentence must do, both learned the hard way. It states the resulting run state
 * itself, because a bare "concurrency is now 15" is the only answer in the whole tool set that
 * ends without saying where things stand -- and a model with nothing to close its turn on goes
 * looking for an action, which is how a plain concurrency change ended in a rejected start. And
 * it says what is true rather than what not to do: an earlier wording ("do not restart it") was
 * the one negated instruction anywhere in this API, and naming the very action it forbids is not
 * how a small model reads it.
 */
[[nodiscard]] std::string concurrencySentence(
    RunState state, const ActivityNames& names, unsigned int count);

/**
 * @brief Off / dry-run / active tri-state shared by draw and resign adjudication.
 *
 * Mirrors ImGuiControls::triStateInput(): Test evaluates and logs what it would have decided
 * without ending games, Active actually adjudicates. Maps to the underlying active/testOnly bool
 * pair via applyAdjudicationMode() / adjudicationModeOf() below.
 */
enum class AdjudicationMode { Off, Test, Active };

/**
 * @brief Writes an AdjudicationMode into the active/testOnly bool pair the adjudication configs
 * actually store. Shared by the tournament and SPRT actions so the mapping exists exactly once.
 */
void applyAdjudicationMode(AdjudicationMode mode, bool& active, bool& testOnly);

/** @brief Reads an active/testOnly bool pair back as an AdjudicationMode. */
[[nodiscard]] AdjudicationMode adjudicationModeOf(bool active, bool testOnly);

/** @brief Names an AdjudicationMode for a status report ("off" / "test" / "active"). */
[[nodiscard]] std::string adjudicationModeName(AdjudicationMode mode);

/**
 * @brief What the caller would have to do differently, for the failures where that is worth saying.
 *
 * A failed action often knows the way out, but not how to phrase it: "let the user browse for the
 * file instead" is a fact about the GUI, whereas "set epd_file_dialog=true" names a parameter that
 * only the model-facing API has. So the action names the remedy and the mapper (see
 * remedyInstruction() in src/llm/api/llm-tool-api.h) puts it into the words of whatever external
 * API is current -- which keeps a renamed parameter from quietly turning good advice into a
 * dangling reference.
 */
enum class Remedy {
    None,
    AskUserForOpeningsFile, ///< The openings book path is unusable; have the user pick one.
    AskUserForEpdFile,      ///< The EPD file path is unusable; have the user pick one.
    ClearEpdResultFirst     ///< The previous EPD run has to be discarded before a new one can start.
};

/**
 * @brief Outcome of one internal action.
 *
 * `text` describes what happened in plain, user-readable prose -- the external layer decides what
 * to do with it (today: hand it to the model *and* show it in the chat, see GuiToolResult).
 */
struct ActionResult {
    /** @brief Whether the action did what was asked. Failure is a normal, reportable outcome. */
    bool ok = true;

    /** @brief What happened, as one or a few short sentences. Never raw data or internal ids. */
    std::string text;

    /**
     * @brief Optional live GUI control to show instead of `text`'s data (see
     * GuiToolResult::renderWidget for the threading contract this ends up under).
     *
     * Set by actions whose whole point is to *show* something, so the result appears in the chat
     * as a real control rather than a text dump.
     */
    std::function<void()> widget{};

    /**
     * @brief Signals that the user has already been interrupted by this action (a native dialog
     * stole focus), so nothing should follow it.
     *
     * The external layer turns this into GuiToolResult::terminal, which ends the agent turn --
     * see that field for why suppressing the model's commentary has to be mechanical rather than
     * merely instructed.
     */
    bool endsTurn = false;

    /** @brief What would have to be done differently, when this failed and there is a way out. */
    Remedy remedy = Remedy::None;
};

/** @brief Convenience for a successful ActionResult carrying only text. */
[[nodiscard]] inline ActionResult succeeded(std::string text) {
    return ActionResult{.ok = true, .text = std::move(text)};
}

/** @brief Convenience for a failed ActionResult carrying only text. */
[[nodiscard]] inline ActionResult failed(std::string text) {
    return ActionResult{.ok = false, .text = std::move(text)};
}

/** @brief Joins values with ", " -- used throughout the actions layer to list names/changes. */
[[nodiscard]] std::string joinList(const std::vector<std::string>& values);

} // namespace QaplaLlm::Actions
