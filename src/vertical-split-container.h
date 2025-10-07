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
#include "snackbar.h"
#include <memory>
#include <algorithm>
#include <functional>
#include <string>

#include <imgui.h>

namespace QaplaWindows {

    /**
     * @brief Displays two embedded windows vertically with a draggable splitter.
     */
    class VerticalSplitContainer : public EmbeddedWindow {
    public:

        /**
         * @brief Constructs a vertical split container with specified window flags
         * @param name The unique identifier for this split container
         * @param top ImGui window flags for the top child window
         * @param bottom ImGui window flags for the bottom child window
         */
        VerticalSplitContainer(
            const std::string& name,
            ImGuiWindowFlags top = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse, 
            ImGuiWindowFlags bottom = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        /**
         * @brief Sets the embedded window for the top panel
         * @param window Unique pointer to the window to be displayed in the top panel
         */
        void setTop(std::unique_ptr<EmbeddedWindow> window);

        /**
         * @brief Sets the embedded window for the bottom panel
         * @param window Unique pointer to the window to be displayed in the bottom panel
         */
        void setBottom(std::unique_ptr<EmbeddedWindow> window);

        /**
         * @brief Sets a callback function for drawing the top panel
         * @param callback Function to be called when drawing the top panel
         */
        void setTop(std::function<void()> callback);

        /**
         * @brief Sets a callback function for drawing the bottom panel
         * @param callback Function to be called when drawing the bottom panel
         */
        void setBottom(std::function<void()> callback);

        /**
         * @brief Sets a preset height for either the top or bottom panel
         * @param height The height to set in pixels
         * @param isTop If true, sets height for top panel; if false, sets height for bottom panel
         */
        void setPresetHeight(float height, bool isTop);

        /**
         * @brief Sets a fixed height for either the top or bottom panel
         * @param height The fixed height in pixels
         * @param isTop If true, fixes the top panel height; if false, fixes the bottom panel height
         */
        void setFixedHeight(float height, bool isTop);

        /**
         * @brief Sets the minimum height for the top window
         * @param height The minimum height in pixels
         */
        void setMinTopHeight(float height);

        /**
         * @brief Sets the minimum height for the bottom window
         * @param height The minimum height in pixels
         */
        void setMinBottomHeight(float height);

        /**
         * @brief Renders the split container with both panels and the splitter
         * Overrides the base class draw method to render the vertical split layout
         */
        void draw() override;

    private:
        /**
         * @brief Computes the appropriate height for the top panel based on available space and constraints
         * @param avail Available space vector (width and height)
         * @return The computed height for the top panel
         */
        float computeTopHeight(ImVec2 avail);

        /**
         * @brief Renders the draggable splitter between the top and bottom panels
         * @param id Unique identifier for the splitter widget
         * @param size Size vector for the splitter (width and height)
         */
        void drawSplitter(const std::string& id, const ImVec2& size);

        /** @brief Embedded window displayed in the top panel */
        std::unique_ptr<EmbeddedWindow> topWindow_;
        
        /** @brief Embedded window displayed in the bottom panel */
        std::unique_ptr<EmbeddedWindow> bottomWindow_;

        /** @brief Callback function for drawing the top panel */
        std::function<void()> topCallback_;
        
        /** @brief Callback function for drawing the bottom panel */
        std::function<void()> bottomCallback_;

        /** @brief ImGui window flags for the top child window */
        ImGuiWindowFlags topFlags_ = ImGuiWindowFlags_None;
        
        /** @brief ImGui window flags for the bottom child window */
        ImGuiWindowFlags bottomFlags_ = ImGuiWindowFlags_None;

        /** @brief Unique identifier for this split container */
        std::string name_;

        /** @brief Height of the splitter in pixels */
        const float splitterHeight_ = 5.0F;

        /** @brief Minimum height for the top panel in pixels */
        float minTopHeight_ = 100.0F;
        
        /** @brief Minimum height for the bottom panel in pixels */
        float minBottomHeight_ = 100.0F;

        /** @brief Current height of the top panel in pixels */
        float topHeight_ = 500.0F;
        
        /** @brief Current height of the bottom panel in pixels */
        float bottomHeight_ = 0.0F;
        
        /** @brief Preset height for the top panel (0 if not set) */
        float topPresetHeight_ = 0.0F;
        
        /** @brief Preset height for the bottom panel (0 if not set) */
        float bottomPresetHeight_ = 0.0F;
        
        /** @brief Previous available height for delta calculations */
        float availY_ = 0.0F;

        /** @brief Whether the top panel has a fixed height */
        bool topFixed_ = false;
        
        /** @brief Whether the bottom panel has a fixed height */
        bool bottomFixed_ = false;
    };

} // namespace QaplaWindows
