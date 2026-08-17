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
#include "gui-action-clop.h"
#include "gui-action-epd.h"
#include "gui-action-sprt.h"
#include "gui-action-tournament.h"
#include "../activity-watch.h"

#include <string>
#include <utility>
#include <vector>

namespace QaplaLlm::Actions {

namespace {
    /**
     * @brief Tells the watch what an action just did, without waiting for the next frame.
     *
     * The frame loop samples all three every frame anyway (see initializeLlmChat), which catches
     * a run ending on its own or a user stopping one by hand. What it cannot catch in time is the
     * sequence the wait exists for: start a run, then wait on it. Both calls land between two
     * frames, so the watch would still have been showing "idle" and the wait would have answered
     * "nothing is running" about a run that had just begun.
     */
    void publishProgress(Activity activity) {
        QaplaLlm::ActivityWatch::instance().update(activity, activityProgress(activity));
    }
} // namespace

ActionResult startActivity(Activity activity) {
    auto result = [&]() {
        switch (activity) {
            case Activity::Sprt: return startSprt();
            case Activity::Clop: return startClop();
            case Activity::Epd: return startEpd();
            case Activity::Tournament:
            default: return startTournament();
        }
    }();
    publishProgress(activity);
    return result;
}

ActionResult stopActivity(Activity activity, StopMode mode) {
    auto result = [&]() {
        switch (activity) {
            case Activity::Sprt: return stopSprt(mode);
            case Activity::Clop: return stopClop(mode);
            case Activity::Epd: return stopEpd(mode);
            case Activity::Tournament:
            default: return stopTournament(mode);
        }
    }();
    publishProgress(activity);
    return result;
}

ActionResult activityStatus(Activity activity) {
    switch (activity) {
        case Activity::Sprt: return sprtStatus();
        case Activity::Clop: return clopStatus();
        case Activity::Epd: return epdStatus();
        case Activity::Tournament:
        default: return tournamentStatus();
    }
}

ActivityProgress activityProgress(Activity activity) {
    switch (activity) {
        case Activity::Sprt: return sprtProgress();
        case Activity::Clop: return clopProgress();
        case Activity::Epd: return epdProgress();
        case Activity::Tournament:
        default: return tournamentProgress();
    }
}

ActionResult clearActivityResult(Activity activity) {
    auto result = [&]() {
        switch (activity) {
            case Activity::Sprt: return clearSprtResult();
            case Activity::Clop: return clearClopResult();
            case Activity::Epd: return clearEpdResult();
            case Activity::Tournament:
            default: return clearTournamentResult();
        }
    }();
    publishProgress(activity); // it stops a running activity first -- see publishProgress()
    return result;
}

ActionResult showActivityResult(Activity activity) {
    switch (activity) {
        case Activity::Sprt: return showSprtResult();
        case Activity::Clop: return showClopResult();
        case Activity::Epd: return showEpdResult();
        case Activity::Tournament:
        default: return showTournamentResult();
    }
}

std::string runningActivitiesText() {
    // An idle activity reports "" (see tournamentActivityText() and friends) and is dropped here,
    // rather than each of the three carrying an "is idle" wording that only reads well alone.
    std::vector<std::string> active;
    for (auto& text : {tournamentActivityText(), sprtActivityText(), epdActivityText(),
             clopActivityText()}) {
        if (!text.empty()) {
            active.push_back(text);
        }
    }

    if (!active.empty()) {
        std::string joined = active.front();
        for (std::size_t i = 1; i < active.size(); ++i) {
            joined += "; " + active[i];
        }
        return "Currently: " + joined + ".";
    }

    // With nothing running, "nothing is running" on its own is the least useful true sentence
    // there is: it leaves a caller unable to tell an activity that is one call away from starting
    // from one that has never been set up. So the same answer names the ones that could go right
    // now -- and only those. The rest are simply left out rather than listed as unready, for the
    // reason given at readyToStartSentence(): a named obstacle is read as an assignment.
    std::vector<std::string> ready;
    const std::pair<const ActivityNames&, bool> entries[] = {
        {TOURNAMENT_NAMES, tournamentIsReadyToStart()},
        {SPRT_NAMES, sprtIsReadyToStart()},
        {EPD_NAMES, epdIsReadyToStart()}, {CLOP_NAMES, clopIsReadyToStart()}};
    for (const auto& [names, isReady] : entries) {
        if (isReady) {
            ready.emplace_back(names.bare);
        }
    }

    std::string text = "Nothing is currently running.";
    if (!ready.empty()) {
        text += " Ready to start exactly as configured: " + joinList(ready) + ".";
    }
    return text;
}

} // namespace QaplaLlm::Actions
