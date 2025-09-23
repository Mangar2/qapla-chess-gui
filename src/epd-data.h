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
#include <optional>

class EpdManager;
struct EpdTestCase;
struct EpdTestResult;

namespace QaplaWindows {


	class EpdData {
    public: 
        EpdData();
        ~EpdData();

        void init();

        /**
		 * @brief Polls the EPD data for new entries.
		 */
        void pollData();

        /**
         * @brief Starts the analysis of the EPD test set.
         * This method will load the EPD file and start the analysis with the configured engines.
		 */
        void analyse() const;

        /**
		 * @brief Clears the current analysis results.
		 */
        void clear();

        /**
         * @brief Draws the EPD test results table.
         * @param size Size of the table to draw.
         * @return The index of the selected row, or std::nullopt if no row was selected.
		 */
        std::optional<size_t> drawTable(const ImVec2& size);
        
        struct EpdConfig {
            std::string filepath;
            std::vector<EngineConfig> engines;
            uint32_t concurrency;
            uint64_t maxTimeInS;
            uint64_t minTimeInS;
            uint32_t seenPlies;
        };
        EpdConfig& config() {
            return epdConfig_;
		}

        /**
         * @brief Retrieves the FEN string for a given index in the results.
         * @param index Index of the test case in the results.
         * @return Optional containing the FEN string if available, otherwise std::nullopt.
		 */
        std::optional<std::string> getFen(size_t index) const;

	private:

        EpdConfig epdConfig_;
        uint64_t updateCnt = 0;
        void populateTable();

		std::shared_ptr<EpdManager> epdManager_;
		std::unique_ptr<std::vector<EpdTestResult>> epdResults_;

        ImGuiTable table_;

    };

}
