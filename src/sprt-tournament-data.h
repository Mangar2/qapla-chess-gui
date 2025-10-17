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

#include "imgui-engine-select.h"

#include <memory>
#include <vector>

namespace QaplaWindows {

    class SprtTournamentData {
    public:
        SprtTournamentData();
        ~SprtTournamentData();

        static SprtTournamentData& instance() {
            static SprtTournamentData instance;
            return instance;
        }

        /**
         * @brief Returns a reference to the engine selection.
         * @return Reference to the engine selection.
         */
        ImGuiEngineSelect& engineSelect() {
            return *engineSelect_;
        }

        const ImGuiEngineSelect& engineSelect() const {
            return *engineSelect_;
        }

        /**
         * @brief Set the Engine Configurations object.
         * 
         * @param configurations Vector with all engine configurations 
         */
        void setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations);

    private:
        void setupCallbacks();
        void loadEngineSelectionConfig();

        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        std::vector<ImGuiEngineSelect::EngineConfiguration> engineConfigurations_;
    };

}
