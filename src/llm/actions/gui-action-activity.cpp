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

#include "gui-action-activity.h"
#include "gui-action-epd.h"
#include "gui-action-sprt.h"
#include "gui-action-tournament.h"

#include <vector>

namespace QaplaLlm::Actions {

ActionResult startActivity(Activity activity) {
    switch (activity) {
        case Activity::Sprt: return startSprt();
        case Activity::Epd: return startEpd();
        case Activity::Tournament:
        default: return startTournament();
    }
}

ActionResult stopActivity(Activity activity, StopMode mode) {
    switch (activity) {
        case Activity::Sprt: return stopSprt(mode);
        case Activity::Epd: return stopEpd(mode);
        case Activity::Tournament:
        default: return stopTournament(mode);
    }
}

ActionResult activityStatus(Activity activity) {
    switch (activity) {
        case Activity::Sprt: return sprtStatus();
        case Activity::Epd: return epdStatus();
        case Activity::Tournament:
        default: return tournamentStatus();
    }
}

ActionResult clearActivityResult(Activity activity) {
    switch (activity) {
        case Activity::Sprt: return clearSprtResult();
        case Activity::Epd: return clearEpdResult();
        case Activity::Tournament:
        default: return clearTournamentResult();
    }
}

ActionResult showActivityResult(Activity activity) {
    switch (activity) {
        case Activity::Sprt: return showSprtResult();
        case Activity::Epd: return showEpdResult();
        case Activity::Tournament:
        default: return showTournamentResult();
    }
}

std::string runningActivitiesText() {
    // An idle activity reports "" (see tournamentActivityText() and friends) and is dropped here,
    // rather than each of the three carrying an "is idle" wording that only reads well alone.
    std::vector<std::string> active;
    for (auto& text : {tournamentActivityText(), sprtActivityText(), epdActivityText()}) {
        if (!text.empty()) {
            active.push_back(text);
        }
    }

    if (active.empty()) {
        return "Nothing is currently running -- no tournament, no SPRT test, no EPD analysis.";
    }
    std::string joined = active.front();
    for (std::size_t i = 1; i < active.size(); ++i) {
        joined += "; " + active[i];
    }
    return "Currently: " + joined + ".";
}

} // namespace QaplaLlm::Actions
