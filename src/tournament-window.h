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
#include "imgui-engine-global-settings.h"

#include "qapla-tester/engine-config.h"

#include <memory>

namespace QaplaWindows {

    class ImGuiTable;

    /**
     * @brief Displays the move list with associated search data for a game.
     */
    class TournamentWindow: public EmbeddedWindow {
    public:
        /**
         * @brief Sets the data source for this window.
         * @param record Shared pointer to the constant game record.
         */
        TournamentWindow();
        ~TournamentWindow();

        void draw() override;
        
        /**
         * @brief Sets the callback that is called when the engine configuration changes.
         * @param callback The new callback.
         */
        void setEngineConfigurationCallback(ImGuiEngineSelect::ConfigurationChangedCallback callback);



    private:
        static void drawButtons();
        static void executeCommand(const std::string &button);
        bool drawInput();

        /**
         * @brief Sets the engine configurations from INI file sections
         */
        void setEngineConfiguration();

        /**
         * @brief Sets the global settings configuration from INI file
         */
        void setGlobalSettingsConfiguration();
        
        std::unique_ptr<ImGuiEngineSelect> engineSelect_;
        std::unique_ptr<ImGuiEngineGlobalSettings> globalSettings_;


    };

} // namespace QaplaWindows
