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

#include <string>
#include <vector>

/**
 * @file
 * @brief Engine catalog logic, free of both tool layers and of the GUI stack.
 *
 * Same reasoning as gui-tool-tournament.h: pure enough for the unit-tests target to link
 * directly, so it stays out of src/llm/actions/ (which is GUI-only) and out of src/llm/tools/
 * (which is model-facing only).
 */

namespace QaplaLlm {

/**
 * @brief Returns the globally configured engines (name + protocol) as a JSON array string.
 *
 * Pure/UI-independent so it can be unit-tested directly; this is exactly what the
 * listInstalledEngines() action reports.
 */
[[nodiscard]] std::string listInstalledEnginesJson();

/**
 * @brief Outcome of adding engines from a set of executable paths.
 */
struct AddEnginesOutcome {
    std::vector<std::string> addedNames;
    std::vector<std::string> duplicateNames; ///< Already present (same cmd + protocol); left untouched.
};

/**
 * @brief Adds engines from the given executable paths to the global engine
 * catalog, skipping ones already configured (same cmd + protocol).
 *
 * Pure/UI-independent so it can be unit-tested with fabricated paths (no
 * filesystem access is required to build an EngineConfig from a path); the
 * "open_add_engine_dialog" tool handler is a thin wrapper that only adds
 * the native file picker on top of this.
 */
[[nodiscard]] AddEnginesOutcome addEnginesFromPaths(const std::vector<std::string>& paths);

} // namespace QaplaLlm
