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
 * @brief Lists the GUI's global engine catalog: every configured engine's name and protocol.
 *
 * The listing itself is produced by listInstalledEnginesJson() (gui-tool-engine-management.h),
 * which is pure and unit-tested there.
 */
[[nodiscard]] ActionResult listInstalledEngines();

/**
 * @brief Asks the user to pick engine executables and adds them to the global catalog.
 *
 * Capability detection (protocol, options) runs synchronously before this returns, so the result
 * already reflects the final outcome rather than promising a later one. Always ends the turn (see
 * ActionResult::endsTurn): a native picker has stolen focus and is closed again by the time
 * anything could be said about it.
 */
[[nodiscard]] ActionResult addEnginesViaDialog();

} // namespace QaplaLlm::Actions
