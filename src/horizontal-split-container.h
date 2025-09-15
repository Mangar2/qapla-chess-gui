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
#include <imgui.h>

namespace QaplaWindows {

    /**
     * @brief Displays two embedded windows side by side with a draggable splitter.
     */
    class HorizontalSplitContainer : public EmbeddedWindow {
    public:

        /**
         * @brief Constructs a horizontal split container with specified window flags
         * @param name The unique identifier for this split container
         * @param left ImGui window flags for the left child window
         * @param right ImGui window flags for the right child window
         */
        HorizontalSplitContainer(
            const std::string& name,
            ImGuiWindowFlags left = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse, 
            ImGuiWindowFlags right = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        /**
         * @brief Sets the embedded window for the left panel
         * @param window Unique pointer to the window to be displayed in the left panel
         */
        void setLeft(std::unique_ptr<EmbeddedWindow> window);

        /**
         * @brief Sets the embedded window for the right panel
         * @param window Unique pointer to the window to be displayed in the right panel
         */
        void setRight(std::unique_ptr<EmbeddedWindow> window);

        /**
         * @brief Sets a callback function for drawing the left panel
         * @param callback Function to be called when drawing the left panel
         */
        void setLeft(std::function<void()> callback);

        /**
         * @brief Sets a callback function for drawing the right panel
         * @param callback Function to be called when drawing the right panel
         */
        void setRight(std::function<void()> callback);

        /**
         * @brief Sets a preset width for either the left or right panel
         * @param width The width to set in pixels
         * @param isLeft If true, sets width for left panel; if false, sets width for right panel
         */
        void setPresetWidth(float width, bool isLeft);

        /**
         * @brief Sets a fixed width for either the left or right panel
         * @param width The fixed width in pixels
         * @param isLeft If true, fixes the left panel width; if false, fixes the right panel width
         */
        void setFixedWidth(float width, bool isLeft);

        /**
         * @brief Renders the split container with both panels and the splitter
         * Overrides the base class draw method to render the horizontal split layout
         */
        void draw() override;

    private:
        /**
         * @brief Computes the appropriate width for the left panel based on available space and constraints
         * @param avail Available space vector (width and height)
         * @return The computed width for the left panel
         */
        float computeLeftWidth(ImVec2 avail);

        /**
         * @brief Renders the draggable splitter between the left and right panels
         * @param id Unique identifier for the splitter widget
         * @param vec Size vector for the splitter (width and height)
         */
        void drawSplitter(const std::string& id, const ImVec2& vec);
        
        /** @brief Width of the splitter in pixels */
        const float splitterWidth_ = 5.0f;
        
        /** @brief Minimum size for each panel in pixels */
        const float minSize_ = 100.0f;

        /** @brief Embedded window displayed in the left panel */
        std::unique_ptr<EmbeddedWindow> leftWindow_;
        
        /** @brief Embedded window displayed in the right panel */
        std::unique_ptr<EmbeddedWindow> rightWindow_;

        /** @brief Callback function for drawing the left panel */
        std::function<void()> leftCallback_;
        
        /** @brief Callback function for drawing the right panel */
        std::function<void()> rightCallback_;
        
        /** @brief ImGui window flags for the left child window */
        ImGuiWindowFlags leftFlags_ = ImGuiWindowFlags_None;
        
        /** @brief ImGui window flags for the right child window */
        ImGuiWindowFlags rightFlags_ = ImGuiWindowFlags_None;

        /** @brief Unique identifier for this split container */
        std::string name_;

        /** @brief Current width of the left panel in pixels */
        float leftWidth_ = 0.0f;
        
        /** @brief Current width of the right panel in pixels */
        float rightWidth_ = 0.0f;
        
        /** @brief Preset width for the right panel (0 if not set) */
        float rightPresetWidth_ = 0.0f;
        
        /** @brief Preset width for the left panel (0 if not set) */
        float leftPresetWidth_ = 0.0f;
        
        /** @brief Previous available width for delta calculations */
        float availX_ = 0.0f;
        
        /** @brief Whether the left panel has a fixed width */
        bool leftFixed_ = false;
        
        /** @brief Whether the right panel has a fixed width */
        bool rightFixed_ = false;
    };

} // namespace QaplaWindows
