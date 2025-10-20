/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "tutorial.h"
#include "snackbar.h"

bool Tutorial::isCompleted(Topic topic) const {
    switch (topic) {
        case Topic::Snackbar:
            return SnackbarManager::instance().getTutorialCounter() >= getCompletionThreshold(topic);
        default:
            return false;
    }
}

void Tutorial::restartTopic(Topic topic) {
    switch (topic) {
        case Topic::Snackbar:
            SnackbarManager::instance().resetTutorialCounter();
            break;
        default:
            break;
    }
}

std::string Tutorial::getTopicName(Topic topic) {
    switch (topic) {
        case Topic::Snackbar:
            return "Snackbar System";
        default:
            return "Unknown";
    }
}

uint32_t Tutorial::getCompletionThreshold(Topic topic) {
    switch (topic) {
        case Topic::Snackbar:
            return 3; // Completed after 3 steps
        default:
            return 1;
    }
}
