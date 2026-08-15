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

// Only the qapla executable links this: OsDialogs and Configuration transitively pull in the
// ImGui/GLFW stack. The pure part (addEnginesFromPaths/listInstalledEnginesJson) lives in
// gui-tool-engine-management.cpp and is unit-tested there.

#include "gui-action-engines.h"
#include "../gui-tool-engine-management.h"
#include "../../os-dialogs.h"
#include "../../configuration.h"

namespace QaplaLlm::Actions {

ActionResult listInstalledEngines() {
    return succeeded(listInstalledEnginesJson());
}

ActionResult addEnginesViaDialog() {
    auto paths = QaplaWindows::OsDialogs::openFileDialog(true);
    if (paths.empty()) {
        return ActionResult{.ok = true,
            .text = "The user cancelled the dialog; no engine was added.",
            .widget = nullptr, .endsTurn = true};
    }

    auto outcome = addEnginesFromPaths(paths);
    if (!outcome.addedNames.empty()) {
        auto& configuration = QaplaConfiguration::Configuration::instance();
        configuration.setModified();
        // Synchronous and blocking (not the fire-and-forget autoDetect() the "Detect" button
        // uses): detection is normally sub-second per engine, and this result must reflect the
        // *actual* final outcome, not a promise of one -- any out-of-band "done" signal is not
        // correlated with this call, so it could arrive before or long after the result does.
        configuration.getEngineCapabilities().autoDetectSync();
    }

    std::string message;
    if (!outcome.addedNames.empty()) {
        message += "Added and detected: " + joinList(outcome.addedNames) + ". Ready to use.";
    }
    if (!outcome.duplicateNames.empty()) {
        if (!message.empty()) {
            message += "\n";
        }
        message += "Already configured (skipped): " + joinList(outcome.duplicateNames) + ".";
    }
    if (message.empty()) {
        message = "No engines were added.";
    }
    return ActionResult{.ok = true, .text = message, .widget = nullptr, .endsTurn = true};
}

} // namespace QaplaLlm::Actions
