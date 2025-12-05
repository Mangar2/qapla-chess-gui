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

#pragma once

#include "tutorial.h"

#include <string>
#include <cstdint>

namespace QaplaWindows {

    class EpdData;

    /**
     * @brief ImGui component for rendering EPD configuration settings.
     * 
     * This class handles the UI for EPD-specific settings like seen plies,
     * max/min time, and file path.
     */
    class ImGuiEpdConfiguration {
    public:
        /**
         * @brief Options to control which UI elements are displayed.
         */
        struct DrawOptions {
            bool alwaysOpen = false;      ///< Whether the configuration panel is always open
            bool showSeenPlies = true;    ///< Show seen plies input
            bool showMaxTime = true;      ///< Show max time input
            bool showMinTime = true;      ///< Show min time input
            bool showFilePath = true;     ///< Show file path input
        };

        ImGuiEpdConfiguration() = default;
        ~ImGuiEpdConfiguration() = default;

        /**
         * @brief Renders the EPD configuration UI.
         * @param options Options controlling which elements to display.
         * @param inputWidth Width for input controls.
         * @param indent Indentation level.
         * @param tutorialContext Tutorial context with highlighting and annotations.
         * @return True if any value was changed, false otherwise.
         */
        bool draw(const DrawOptions& options, float inputWidth, float indent,
            const Tutorial::TutorialContext& tutorialContext = Tutorial::TutorialContext{});

    private:
        /**
         * @brief Updates the configuration in the Configuration singleton.
         */
        void updateConfiguration() const;
    };

}
