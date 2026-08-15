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

#include "gui-action-types.h"

namespace QaplaLlm::Actions {

std::string joinList(const std::vector<std::string>& values) {
    std::string joined;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            joined += ", ";
        }
        joined += values[i];
    }
    return joined;
}

void applyAdjudicationMode(AdjudicationMode mode, bool& active, bool& testOnly) {
    active = mode != AdjudicationMode::Off;
    testOnly = mode == AdjudicationMode::Test;
}

AdjudicationMode adjudicationModeOf(bool active, bool testOnly) {
    if (!active) {
        return AdjudicationMode::Off;
    }
    return testOnly ? AdjudicationMode::Test : AdjudicationMode::Active;
}

std::string adjudicationModeName(AdjudicationMode mode) {
    switch (mode) {
        case AdjudicationMode::Test: return "test";
        case AdjudicationMode::Active: return "active";
        case AdjudicationMode::Off:
        default: return "off";
    }
}

} // namespace QaplaLlm::Actions
