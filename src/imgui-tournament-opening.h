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

#include "qapla-tester/openings.h"
#include "qapla-tester/ini-file.h"

#include <string>
#include <optional>
#include <cstdint>
#include <vector>

namespace QaplaWindows {

    class ImGuiTournamentOpening {
    public:
        ImGuiTournamentOpening() = default;
        ~ImGuiTournamentOpening() = default;

        bool draw(float inputWidth, float fileInputWidth, float indent);

        void setId(const std::string& id) {
            id_ = id;
        }

        Openings& openings() {
            return openings_;
        }

        [[nodiscard]] const Openings& openings() const {
            return openings_;
        }

        void loadConfiguration();
        [[nodiscard]] std::vector<QaplaHelpers::IniFile::Section> getSections() const;

    private:
        Openings openings_;
        std::string id_;
    };

}
