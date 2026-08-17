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

/** @brief Discards the given activity's results, stopping it first if it is still running. */
[[nodiscard]] ActionResult clearActivityResult(Activity activity);

/** @brief Shows the given activity's results as a live control in the chat. */
[[nodiscard]] ActionResult showActivityResult(Activity activity);

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
