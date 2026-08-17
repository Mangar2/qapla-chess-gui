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
#include "../gui-tool-engine-management.h"

#include <string>
#include <vector>

namespace QaplaLlm::Actions {

/**
 * @brief Which set of engine configurations an option change applies to.
 *
 * Not two kinds of thing, but the same kind in different sets: an engine configuration is a
 * complete, self-contained value, and the target says which set the one being changed lives in.
 * The catalog is what every new selection is copied *from*; a run holds its own copies, which is
 * what makes an SPRT of one build against itself under two option sets possible at all.
 */
enum class EngineTarget { Catalog, Tournament, Sprt, Epd };

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

/**
 * @brief Adds named engines from executable paths to the global catalog and detects them.
 *
 * Detection is part of installing, not a step of its own: an engine whose options are unknown
 * cannot be configured, and every caller that installs one is about to configure it. It runs
 * synchronously, so the result already says what the engines can do rather than promising a later
 * answer -- the same reasoning as addEnginesViaDialog(), which this shares its second half with.
 *
 * Unlike that one it needs no user at the window, which is what makes it the path a remote caller
 * uses. It does mean the caller has to know where the file is; a path that does not exist is
 * reported rather than added, since an unusable entry in the catalog outlives the mistake.
 *
 * See addNamedEngines() for why the name is the caller's to choose here and not derived from the
 * filename: two builds of one engine would otherwise end up sharing a name.
 */
[[nodiscard]] ActionResult installEngines(const std::vector<NamedEnginePath>& engines);

/**
 * @brief Reports one catalog engine's configuration and the options its program supports.
 *
 * The report itself is built by engineDetailsText() (gui-tool-engine-management.h); this resolves
 * the name against the catalog first, with the same tolerance for a partial name that engine
 * selection has.
 */
[[nodiscard]] ActionResult engineDetails(const std::string& name);

/**
 * @brief Sets UCI option values on one engine configuration in the given set.
 *
 * Values are checked against what the engine reported it supports, and an unusable one is refused
 * rather than stored -- see applyEngineOptions() for why silence would be worse than a refusal.
 *
 * For a run target this changes only that run's copy, so it has to come *after* the engines are
 * selected: selecting copies the engine fresh out of the catalog and takes its option values with
 * it. For the catalog it changes what future selections start from, and leaves every run that
 * already holds a copy alone.
 */
[[nodiscard]] ActionResult setEngineOptions(EngineTarget target, const std::string& engineName,
    const std::vector<EngineOptionAssignment>& options);

} // namespace QaplaLlm::Actions
