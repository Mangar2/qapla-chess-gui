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

#include "game-manager/adjudication-manager.h"
#include "base-elements/ini-file.h"

#include <string>
#include <vector>

namespace QaplaWindows {

    class ImGuiTournamentAdjudication {
    public:
        ImGuiTournamentAdjudication() = default;
        ~ImGuiTournamentAdjudication() = default;

        /**
         * @brief Renders the tournament adjudication configuration UI.
         * @param inputWidth Width for input controls.
         * @param indent Indentation level.
         * @return True if any value was changed, false otherwise.
         */
        bool draw(float inputWidth, float indent);

        void setId(const std::string& id) {
            id_ = id;
        }

        QaplaTester::AdjudicationManager::DrawAdjudicationConfig& drawConfig() {
            return drawConfig_;
        }

        [[nodiscard]] const QaplaTester::AdjudicationManager::DrawAdjudicationConfig& drawConfig() const {
            return drawConfig_;
        }

        QaplaTester::AdjudicationManager::ResignAdjudicationConfig& resignConfig() {
            return resignConfig_;
        }

        [[nodiscard]] const QaplaTester::AdjudicationManager::ResignAdjudicationConfig& resignConfig() const {
            return resignConfig_;
        }

        void loadConfiguration();
        [[nodiscard]] std::vector<QaplaHelpers::IniFile::Section> getSections() const;

    private:
        /**
         * @brief Updates the configuration in the Configuration singleton.
         */
        void updateConfiguration() const;

        QaplaTester::AdjudicationManager::DrawAdjudicationConfig drawConfig_;
        QaplaTester::AdjudicationManager::ResignAdjudicationConfig resignConfig_;
        std::string id_;
    };

}
