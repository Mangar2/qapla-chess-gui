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
#include "callback-manager.h"
#include "imgui-engine-select.h"

#include <memory>
#include <optional>

class EpdManager;
struct EpdTestCase;
struct EpdTestResult;

namespace QaplaWindows {

	class EpdData {
    public: 
        enum class State {
            Starting,
            Running,
            Stopping,
            Stopped,
            Cleared
        };
        struct EpdConfig {
            std::string filepath;
            std::vector<EngineConfig> engines;
            uint32_t concurrency;
            uint64_t maxTimeInS;
            uint64_t minTimeInS;
            uint32_t seenPlies;
        };
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
        void analyse();

        /**
         * @brief Stops the current analysis.
         * @param graceful If true, stops gracefully allowing current tests to finish; if false, stops immediately.
         */ 
        void stopPool(bool graceful);

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
        
        EpdConfig& config() {
            return epdConfig_;
		}
        
        /**
         * @brief Informs the configuration singleton about the current epd configurations
         * 
         */
        void updateConfiguration() const;

        /**
         * @brief Sets the engine configurations for EPD analysis
         * @param configurations Vector with all engine configurations
         */
        void setEngineConfigurations(const std::vector<ImGuiEngineSelect::EngineConfiguration>& configurations);

        /**
         * @brief Retrieves the FEN string for a given index in the results.
         * @param index Index of the test case in the results.
         * @return Optional containing the FEN string if available, otherwise std::nullopt.
		 */
        std::optional<std::string> getFen(size_t index) const;

        /**
         * @brief Sets the selected index in the table.
         * @param index The index to select, or std::nullopt to clear selection.
         */
        void setSelectedIndex(std::optional<size_t> index) {
            selectedIndex_ = index;
        }
        /**
         * @brief Retrieves the currently selected index in the table.
         * @return Optional containing the selected index, or std::nullopt if no selection.
         */
        std::optional<size_t> getSelectedIndex() const {
            return selectedIndex_;
        }

        /**
         * @brief Retrieves the singleton instance of EpdData.
         * @return Reference to the singleton EpdData instance.
         */
        static EpdData& instance() {
            static EpdData instance;
            return instance;
        }

        /**
         * @brief Sets the configurations from INI file sections
         * @param sections A list of INI file sections representing the engine configurations
         */
        void setConfiguration(const QaplaHelpers::IniFile::SectionList& sections);

        State state = State::Cleared;

	private:

        EpdConfig epdConfig_;
        uint64_t updateCnt = 0;
        void populateTable();
        std::optional<size_t> selectedIndex_;
        

		std::shared_ptr<EpdManager> epdManager_;
		std::unique_ptr<std::vector<EpdTestResult>> epdResults_;
   		std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
        uint32_t scheduledEngines_ = 0;

        ImGuiTable table_;

    };

}
