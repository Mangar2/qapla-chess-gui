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

#include <engine-handling/engine-config.h>
#include "imgui-table.h"
#include "callback-manager.h"
#include "imgui-engine-select.h"
#include "autosavable.h"
#include "game-manager-pool-access.h"
#include "viewer-board-window-list.h"

#include <memory>
#include <optional>

namespace QaplaTester {
    class EpdManager;
    struct EpdTestCase;
    struct EpdTestResult;
}

class ImGuiConcurrency;

namespace QaplaWindows {

	class EpdData : public QaplaHelpers::Autosavable {
    public: 
        enum class State {
            Starting,
            Running,
            Stopping,
            Gracefully,
            Stopped,
            Cleared
        };
        struct EpdConfig {
            std::string filepath;
            std::vector<QaplaTester::EngineConfig> engines;
            uint32_t maxConcurrency = 32;
            uint32_t concurrency = 1;
            uint64_t maxTimeInS = 10;
            uint64_t minTimeInS = 1;
            uint32_t seenPlies = 3;
            bool operator==(const EpdConfig& other) const {
                return filepath == other.filepath &&
                       engines == other.engines &&
                       // Concurrency is not compared here, as changing concurrency does not change the configuration itself
                       maxTimeInS == other.maxTimeInS &&
                       minTimeInS == other.minTimeInS &&
                       seenPlies == other.seenPlies;
            }
        };
    public: 
        EpdData();
        ~EpdData();

        void init();
        void setCallbacks();

        /**
		 * @brief Polls the EPD data for new entries.
		 */
        void pollData();

        /**
         * @brief Checks if analysis may be started or continued, and starts it if possible.
         * @param sendMessage If true, shows a message if analysis cannot be started.
         */
        bool mayAnalyze(bool sendMessage = false) const;

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
         * @brief Returns a reference to the engine selection.
         * @return Reference to the engine selection.
         */
        ImGuiEngineSelect& getEngineSelect() {
            return *engineSelect_;
        }
        
        /**
         * @brief Informs the configuration singleton about the current epd configurations
         * 
         */
        void updateConfiguration() const;

        /**
         * @brief Gets the target pool concurrency level.
         * @return The target concurrency level.
         */
        uint32_t getExternalConcurrency() const;

        /**
         * @brief Sets the external concurrency value.
         * @param count The new external concurrency value.
         */
        void setExternalConcurrency(uint32_t count);

        /**
         * @brief Sets the pool concurrency level.
         * @param count The number of concurrent tasks to allow.
         * @param nice If true, reduces the number of active managers gradually.
         * @param direct If true, applies the change immediately without debouncing.
         */
        void setPoolConcurrency(uint32_t count, bool nice = true, bool direct = false);

        /**
         * @brief Sets the GameManagerPool instance to use.
         * @param pool Shared pointer to a GameManagerPool instance.
         */
        void setGameManagerPool(const std::shared_ptr<QaplaTester::GameManagerPool>& pool);

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
     
        /**
         * @brief Checks if the current configuration has changed since the last analysis.
         * @return true if the configuration has changed, false otherwise.
         */
        bool configChanged() const;

        /**
         * @brief Checks if the EPD analysis is currently starting.
         * @return true if the analysis is in the starting state, false otherwise.
         */
        [[nodiscard]] bool isStarting() const {
            return state == State::Starting;
        }

        /**
         * @brief Checks if the EPD analysis is currently running.
         * @return true if the analysis is running, false otherwise.
         */
        [[nodiscard]] bool isRunning() const {
            return state == State::Running;
        }

        /**
         * @brief Checks if the EPD analysis is currently stopping.
         * @return true if the analysis is stopping, false otherwise.
         */
        [[nodiscard]] bool isStopping() const {
            return state == State::Stopping || state == State::Gracefully;
        }

        /**
         * @brief Checks if the EPD analysis is stopped.
         * @return true if the analysis is stopped or cleared, false otherwise.
         */
        [[nodiscard]] bool isStopped() const {
            return state == State::Stopped || state == State::Cleared;
        }

        /**
         * @brief Checks if all EPD tests have been completed.
         * @return true if all tests are finished, false otherwise.
         */
        [[nodiscard]] bool isFinished() const {
            return (state == State::Stopped || state == State::Cleared) && 
                   totalTests > 0 && remainingTests == 0;
        }

        State state = State::Cleared;
        size_t totalTests = 0;
        size_t remainingTests = 0;

        // Inherited from Autosavable: autosave(), saveFile(), loadFile(), setModified()

    protected:
        /**
         * @brief Saves EPD results to the output stream.
         * Overrides Autosavable::saveData.
         * @param out The output stream to write the EPD results to.
         */
        void saveData(std::ofstream& out) override;

        /**
         * @brief Loads EPD results from the input stream.
         * Overrides Autosavable::loadData.
         * @param in The input stream to read the EPD results from.
         */
        void loadData(std::ifstream& in) override;
        

	private:

        EpdConfig epdConfig_;
        EpdConfig scheduledConfig_;
        uint64_t updateCnt_ = 0;

        void populateTable();
        std::optional<size_t> selectedIndex_;

		std::shared_ptr<QaplaTester::EpdManager> epdManager_;
		std::unique_ptr<std::vector<QaplaTester::EpdTestResult>> epdResults_;
   		std::unique_ptr<Callback::UnregisterHandle> pollCallbackHandle_;
        std::unique_ptr<Callback::UnregisterHandle> saveCallbackHandle_;
        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        std::unique_ptr<ImGuiConcurrency> imguiConcurrency_;

        uint32_t scheduledEngines_ = 0;

        ImGuiTable table_;
        GameManagerPoolAccess poolAccess_;
        ViewerBoardWindowList viewerBoardWindows_;


    };

}
