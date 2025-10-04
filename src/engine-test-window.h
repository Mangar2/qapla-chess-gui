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

#include "embedded-window.h"
#include "imgui-engine-select.h"
#include "engine-tests.h"
#include <memory>

namespace QaplaWindows
{
    /**
     * @brief Window for testing chess engines
     */
    class EngineTestWindow : public EmbeddedWindow
    {
    public:
        /**
         * @brief Constructor
         */
        EngineTestWindow();
        ~EngineTestWindow();

        void draw() override;
        
        /**
         * @brief Sets the callback that is called when the engine configuration changes.
         * @param callback The new callback.
         */
        void setEngineConfigurationCallback(ImGuiEngineSelect::ConfigurationChangedCallback callback);
        
        /**
         * @brief Check if Start/Stop test is selected
         */
        bool isStartStopTestSelected() const { return testStartStopSelected_; }
        
        /**
         * @brief Check if Hash Table Memory test is selected
         */
        bool isHashTableMemoryTestSelected() const { return testHashTableMemorySelected_; }
        
        /**
         * @brief Check if Lowercase Option test is selected
         */
        bool isLowerCaseOptionTestSelected() const { return testLowerCaseOptionSelected_; }
        
        /**
         * @brief Check if Engine Options test is selected
         */
        bool isEngineOptionsTestSelected() const { return testEngineOptionsSelected_; }
        
        /**
         * @brief Check if Analyze test is selected
         */
        bool isAnalyzeTestSelected() const { return testAnalyzeSelected_; }
        
        /**
         * @brief Check if Immediate Stop test is selected
         */
        bool isImmediateStopTestSelected() const { return testImmediateStopSelected_; }
        
        /**
         * @brief Check if Infinite Analyze test is selected
         */
        bool isInfiniteAnalyzeTestSelected() const { return testInfiniteAnalyzeSelected_; }

    private:
        void drawButtons();
        void drawInput();
        void drawTests();
        
        /**
         * @brief Sets the engine configurations from INI file sections
         */
        void setEngineConfiguration();
        
        /**
         * @brief Gets the selected engine configurations
         * @return Vector of selected engine configurations
         */
        std::vector<EngineConfig> getSelectedEngineConfigurations() const;
        
        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        
        bool testStartStopSelected_;
        bool testHashTableMemorySelected_;
        bool testLowerCaseOptionSelected_;
        bool testEngineOptionsSelected_;
        bool testAnalyzeSelected_;
        bool testImmediateStopSelected_;
        bool testInfiniteAnalyzeSelected_;
    };

} // namespace QaplaWindows
