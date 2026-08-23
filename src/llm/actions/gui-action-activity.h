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

#include "gui-action-types.h"

#include <optional>
#include <string>

namespace QaplaLlm::Actions {

/**
 * @brief One of the independent things this GUI can have running at once.
 *
 * They share no state: each has its own engine selection, time control, concurrency and results.
 * Every operation below simply forwards to the matching per-activity action, so a caller that
 * already knows which one it wants should call that action directly instead.
 *
 * Clop is the one without a tab of its own -- it is configured and read over the remote control
 * (see QaplaWindows::ClopData) -- but it is an activity like the others in every way that matters
 * here: it runs, it can be stopped, it has results, and its games appear on the boards. Which is
 * why it joins this enum rather than getting a lifecycle of its own: start, stop, status, wait
 * and clear then cover it without another line of external API.
 */
enum class Activity { Tournament, Sprt, Epd, Clop };

/** @brief Starts the given activity. See startTournament() / startSprt() / startEpd(). */
[[nodiscard]] ActionResult startActivity(Activity activity);

/** @brief Stops the given activity. See stopTournament() / stopSprt() / stopEpd(). */
[[nodiscard]] ActionResult stopActivity(Activity activity, StopMode mode);

/** @brief Reports the given activity's full configuration and run state. */
[[nodiscard]] ActionResult activityStatus(Activity activity);

/**
 * @brief What the given activity is doing, in the two facts a waiting caller needs.
 *
 * Cheap enough to ask every frame, which is what feeds QaplaLlm::ActivityWatch.
 */
[[nodiscard]] ActivityProgress activityProgress(Activity activity);

/**
 * @brief The given activity's results as data, or nothing when it has none.
 *
 * The counterpart to activityStatus(): the same numbers, without the sentences around them. A
 * program driving the GUI from outside -- the HTTP remote control, and the tests that use it --
 * needs a value it can compare, not a paragraph whose wording and language are free to change.
 */
[[nodiscard]] std::optional<ResultTable> activityResultTable(Activity activity);

/**
 * @brief Whether the given activity could be started exactly as it now stands.
 *
 * The same fact activityStatus() phrases as a closing sentence for an idle, ready activity.
 */
[[nodiscard]] bool activityIsReadyToStart(Activity activity);

/** @brief Discards the given activity's results, stopping it first if it is still running. */
[[nodiscard]] ActionResult clearActivityResult(Activity activity);

/** @brief Shows the given activity's results as a live control in the chat. */
[[nodiscard]] ActionResult showActivityResult(Activity activity);

/**
 * @brief Writes the given activity to a file at the given path.
 *
 * Tournament and SPRT write the same .qtour/.qsprt state file the window's "Save As" button
 * writes, configuration and results together; EPD writes its per-position results only (see
 * saveEpdToFile()). Clop has no file format of its own and is refused.
 *
 * The path is an argument rather than something a dialog asks for, which is the whole point: the
 * remote control has nobody at a window to answer a file picker.
 */
[[nodiscard]] ActionResult saveActivityToFile(Activity activity, const std::string& file);

/**
 * @brief Reads the given activity back from a file written by saveActivityToFile().
 *
 * Destructive: what the activity held before is replaced. Refused while it runs, and for Clop.
 */
[[nodiscard]] ActionResult loadActivityFromFile(Activity activity, const std::string& file);

/**
 * @brief One sentence naming everything currently running across all three activities.
 *
 * Exists because the three are easy to confuse from the outside -- to someone watching, an SPRT
 * test and an EPD analysis both look exactly like "a tournament running in the background" -- so
 * answering "is anything running?" by checking only one of them would confidently say no while
 * another is in full swing.
 */
[[nodiscard]] std::string runningActivitiesText();

} // namespace QaplaLlm::Actions
