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

namespace QaplaLlm::Actions {

/**
 * @brief Where the PGN file to open comes from.
 *
 * AskUser is the only value that puts a native file picker in front of the user; the other two
 * need no dialog because the path is already known.
 */
enum class PgnSource {
    AskUser,          ///< Native file picker; the user chooses any PGN file.
    TournamentOutput, ///< The file the classic tournament is currently writing to.
    SprtOutput        ///< The file the SPRT test is currently writing to.
};

/**
 * @brief Closes the whole application, exactly as the window's own close button does.
 *
 * The main loop exits normally on its next check and runs its usual shutdown/save sequence, so
 * this is a clean quit, not an abrupt exit.
 */
[[nodiscard]] ActionResult closeApplication();

/**
 * @brief Opens a PGN file in the Pgn tab and switches to it.
 *
 * With PgnSource::AskUser the result is always marked as ending the turn (see
 * ActionResult::endsTurn), cancelled or not: the picker has already stolen focus and is long
 * closed by the time anything could be said about it.
 */
[[nodiscard]] ActionResult openPgnFile(PgnSource source);

} // namespace QaplaLlm::Actions
