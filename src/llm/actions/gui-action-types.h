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
