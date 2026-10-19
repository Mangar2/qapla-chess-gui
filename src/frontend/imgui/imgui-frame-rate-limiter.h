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

#include <string>

struct GLFWwindow;

namespace QaplaWindows {

    /**
     * @brief Adaptive frame rate limiter for ImGui applications
     * 
     * Provides two frame rate modes:
     * - High frame rate: When user is actively interacting (mouse/keyboard)
     * - Low frame rate: When application is idle
     * 
     * Uses a decay counter to smoothly transition between modes after user activity stops.
     */
    class ImGuiFrameRateLimiter {
    public:
        /**
         * @brief Factory method to create frame rate limiter for specific mode
         * @param remoteDesktopMode true for Remote Desktop mode, false for normal mode
         * @return Configured ImGuiFrameRateLimiter instance
         */
        [[nodiscard]] static ImGuiFrameRateLimiter forMode(bool remoteDesktopMode);

        /**
         * @brief Constructor
         * @param lowFps Frame rate when idle (e.g., 8 FPS)
         * @param highFps Frame rate during interaction (e.g., 32 FPS)
         * @param decayFrames Number of frames to maintain high FPS after activity stops
         */
        explicit ImGuiFrameRateLimiter(double lowFps = 8.0, double highFps = 32.0, int decayFrames = 32);

        /**
         * @brief Waits for next frame and updates activity state
         * 
         * Checks for mouse/keyboard activity and adjusts frame rate accordingly.
         * Call this once per frame after ImGui::NewFrame().
         */
        void waitForNextFrame();

        /**
         * @brief Gets current target frame rate
         * @return Current FPS (either low or high depending on activity)
         */
        [[nodiscard]] double getCurrentFps() const;

        /**
         * @brief Checks if currently in high frame rate mode
         * @return true if actively rendering at high FPS
         */
        [[nodiscard]] bool isHighFrameRate() const;

        /**
         * @brief Gets description of current mode configuration
         * @return Human-readable description of low/high FPS settings
         */
        [[nodiscard]] std::string getModeDescription() const;

    private:
        /**
         * @brief Checks for user activity (mouse/keyboard)
         * @return true if user is actively interacting with the application
         */
        [[nodiscard]] bool detectActivity() const;

        /**
         * @brief Updates the activity counter and current frame rate
         */
        void updateActivityState();

        double lowFps_;          ///< Frame rate when idle
        double highFps_;         ///< Frame rate during interaction
        int decayFrames_;        ///< Number of frames to maintain high FPS after activity
        int activityCounter_;    ///< Current countdown value (0 = idle, >0 = active)
        double currentFps_;      ///< Current target frame rate
        double lastFrameTime_;   ///< Timestamp of last frame (GLFW time)
    };

} // namespace QaplaWindows
