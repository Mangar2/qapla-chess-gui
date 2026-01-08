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

#include <base-elements/ini-file.h>

#include <string>
#include <cstdint>
#include <vector>

namespace QaplaTester {
    struct SprtConfig;
}

namespace QaplaWindows {

    /**
     * @brief ImGui component for rendering SPRT configuration settings.
     * 
     * This class handles the UI for SPRT-specific settings like Elo bounds,
     * alpha/beta error rates, and maximum games. It automatically saves
     * configuration changes when the draw() method detects modifications.
     */
    class ImGuiSprtConfiguration {
    public:
        /**
         * @brief Options to control which UI elements are displayed.
         */
        struct DrawOptions {
            // Needed due to clang not accepting default member initializers when used as 
            // default parameters in the same class.
            DrawOptions() : 
                inputWidth(150.0F), indent(10.0F), alwaysOpen(false),
                showEloLower(true), showEloUpper(true), showAlpha(true),
                showBeta(true), showMaxGames(true), showModel(true),
                showPentanomial(true), showCollapsingHeader(true)
            {}

            float inputWidth = 150.0F;       ///< Width of input controls
            float indent = 10.0F;            ///< Indentation for controls
            bool alwaysOpen = false;         ///< Whether the section is always open
            bool showEloLower = true;        ///< Show Elo Lower (H0) input
            bool showEloUpper = true;        ///< Show Elo Upper (H1) input
            bool showAlpha = true;           ///< Show Alpha input
            bool showBeta = true;            ///< Show Beta input
            bool showMaxGames = true;        ///< Show Max Games input
            bool showModel = true;           ///< Show Model selection
            bool showPentanomial = true;     ///< Show Pentanomial checkbox
            bool showCollapsingHeader = true; ///< Whether to show collapsing header
        };

        ImGuiSprtConfiguration() = default;
        ~ImGuiSprtConfiguration() = default;

        /**
         * @brief Renders the SPRT configuration UI.
         * @param options Options controlling which elements to display.
         * @return True if any value was changed, false otherwise.
         */
        bool draw(const DrawOptions& options = DrawOptions{});

        /**
         * @brief Sets the ID for configuration storage.
         * @param id The configuration ID.
         */
        void setId(const std::string& id) {
            id_ = id;
        }

        /**
         * @brief Sets the SPRT configuration pointer.
         * @param config Pointer to the SPRT configuration to use.
         */
        void setConfig(QaplaTester::SprtConfig* config) {
            config_ = config;
        }

        /**
         * @brief Returns a reference to the SPRT configuration.
         * @return Reference to the SPRT configuration.
         */
        QaplaTester::SprtConfig& config() {
            return *config_;
        }

        /**
         * @brief Returns a const reference to the SPRT configuration.
         * @return Const reference to the SPRT configuration.
         */
        [[nodiscard]] const QaplaTester::SprtConfig& config() const {
            return *config_;
        }

        /**
         * @brief Loads configuration from the Configuration singleton.
         */
        void loadConfiguration();

        /**
         * @brief Returns the configuration sections for saving.
         * @return Vector of IniFile sections.
         */
        [[nodiscard]] std::vector<QaplaHelpers::IniFile::Section> getSections() const;

        /**
         * @brief Checks if the current configuration is valid.
         * @return True if configuration is valid, false otherwise.
         */
        [[nodiscard]] bool isValid() const;

    private:
        /**
         * @brief Updates the configuration in the Configuration singleton.
         */
        void updateConfiguration() const;

        /**
         * @brief Draws the Elo Lower (H0) input control.
         * @param inputWidth Width for input control.
         * @return True if the value was changed, false otherwise.
         */
        bool drawEloLower(float inputWidth);

        /**
         * @brief Draws the Elo Upper (H1) input control.
         * @param inputWidth Width for input control.
         * @return True if the value was changed, false otherwise.
         */
        bool drawEloUpper(float inputWidth);

        /**
         * @brief Draws the Alpha input control.
         * @param inputWidth Width for input control.
         * @return True if the value was changed, false otherwise.
         */
        bool drawAlpha(float inputWidth);

        /**
         * @brief Draws the Beta input control.
         * @param inputWidth Width for input control.
         * @return True if the value was changed, false otherwise.
         */
        bool drawBeta(float inputWidth);

        /**
         * @brief Draws the Max Games input control.
         * @param inputWidth Width for input control.
         * @return True if the value was changed, false otherwise.
         */
        bool drawMaxGames(float inputWidth);

        /**
         * @brief Draws the Model selection control.
         * @param inputWidth Width for input control.
         * @return True if the value was changed, false otherwise.
         */
        bool drawModel(float inputWidth);

        /**
         * @brief Draws the Pentanomial checkbox control.
         * @return True if the value was changed, false otherwise.
         */
        bool drawPentanomial();

        QaplaTester::SprtConfig* config_ = nullptr;
        std::string id_;
    };

}
