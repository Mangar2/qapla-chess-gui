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
#include <memory>

namespace QaplaWindows
{

    /**
     * @brief Window for displaying and editing application settings.
     * 
     * This window provides a user interface for modifying various configuration
     * settings of the chess GUI application, including snackbar durations and
     * other global settings.
     */
    class ConfigurationWindow : public EmbeddedWindow
    {
    public:
        /**
         * @brief Constructor
         */
        ConfigurationWindow();
        
        /**
         * @brief Destructor
         */
        ~ConfigurationWindow();

        /**
         * @brief Renders the configuration window
         */
        void draw() override;
        
    private:
        /**
         * @brief Draws the snackbar configuration section
         */
        static void drawSnackbarSettings();
    };

} // namespace QaplaWindows
