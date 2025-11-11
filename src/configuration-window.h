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
#include <string>

namespace QaplaWindows
{
    /**
     * @brief Buffered text input with apply button.
     * 
     * Holds original and current values. Only updates the source value when Apply is clicked.
     * Auto-syncs when source value changes externally.
     */
    class BufferedTextInput {
    public:
        /**
         * @brief Draws the input field with Apply button.
         * @param label Label for the input field.
         * @param sourceValue Reference to the source value (will be updated on Apply).
         * @param width Width of the input field.
         * @return True if Apply was clicked and value was modified.
         */
        bool draw(const char* label, std::string& sourceValue, float width = 200.0F);
        
    private:
        std::string originalValue_;  ///< Original value from source
        std::string currentValue_;   ///< Current edited value
    };

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
        static void drawSnackbarConfig();

        /**
         * @brief Draws the tutorial configuration section
         */
        static void drawTutorialConfig();

        /**
         * @brief Draws the logger configuration section
         */
        void drawLoggerConfig();

        BufferedTextInput reportBaseNameInput_;  ///< Buffered input for report log base name
        BufferedTextInput engineBaseNameInput_;  ///< Buffered input for engine log base name
    };

} // namespace QaplaWindows
