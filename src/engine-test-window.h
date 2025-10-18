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

    private:
        void drawButtons();
        void drawInput();
        void drawReportTables();
        
        /**
         * @brief Sets the engine configurations from INI file sections
         */
        void setEngineConfiguration();
        
        /**
         * @brief Gets the selected engine configurations
         * @return Vector of selected engine configurations
         */
        std::vector<QaplaTester::EngineConfig> getSelectedEngineConfigurations() const;
        
        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
    };

} // namespace QaplaWindows
