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

#include "qapla-tester/engine-config.h"
#include "imgui-table.h"
#include <memory>

class EpdManager;
struct EpdTestCase;
struct EpdTestResult;

namespace QaplaWindows {


	class EpdData {
    public: 
        EpdData();
        ~EpdData();

        void pollData();
        void analyse() const;
        void drawTable(const ImVec2& size) const;
        
        struct EpdConfig {
            std::string filepath;
            EngineConfig engine;
            uint32_t concurrency;
            uint64_t maxTimeInS;
            uint64_t minTimeInS;
            uint32_t seenPlies;
        };
        EpdConfig& config() {
            return epdConfig_;
		}

	private:

        EpdConfig epdConfig_;
        uint64_t updateCnt = 0;
        void populateTable();

		std::shared_ptr<EpdManager> epdManager_;
		std::unique_ptr<std::vector<EpdTestCase>> epdTests_;
		std::unique_ptr<std::vector<EpdTestResult>> epdResults_;

        ImGuiTable table_;

    };

}
