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

#include "imgui-frame-rate-limiter.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <thread>
#include <chrono>
#include <format>

namespace {
    // Frame rate settings for normal mode
    constexpr double NORMAL_LOW_FPS = 16.0;
    constexpr double NORMAL_HIGH_FPS = 64.0;
    
    // Frame rate settings for Remote Desktop mode
    constexpr double RDP_LOW_FPS = 8.0;
    constexpr double RDP_HIGH_FPS = 32.0;
    
    // Decay frames: number of frames to maintain high FPS after activity stops
    constexpr int DECAY_FRAMES = 32;
}

namespace QaplaWindows {

    ImGuiFrameRateLimiter ImGuiFrameRateLimiter::forMode(bool remoteDesktopMode) {
        if (remoteDesktopMode) {
            return ImGuiFrameRateLimiter(RDP_LOW_FPS, RDP_HIGH_FPS, DECAY_FRAMES);
        }
        return ImGuiFrameRateLimiter(NORMAL_LOW_FPS, NORMAL_HIGH_FPS, DECAY_FRAMES);
    }

    ImGuiFrameRateLimiter::ImGuiFrameRateLimiter(double lowFps, double highFps, int decayFrames)
        : lowFps_(lowFps)
        , highFps_(highFps)
        , decayFrames_(decayFrames)
        , activityCounter_(0)
        , currentFps_(lowFps)
        , lastFrameTime_(glfwGetTime())
    {}

    bool ImGuiFrameRateLimiter::detectActivity() const {
        ImGuiIO& guiIo = ImGui::GetIO();
        
        // Mouse movement detected
        if (guiIo.MouseDelta.x != 0.0F || guiIo.MouseDelta.y != 0.0F) {
            return true;
        }
        
        // Mouse buttons pressed
        for (int i = 0; i < IM_ARRAYSIZE(guiIo.MouseDown); ++i) {
            if (guiIo.MouseDown[i]) {
                return true;
            }
        }
        
        // Mouse wheel scrolling
        if (guiIo.MouseWheel != 0.0F || guiIo.MouseWheelH != 0.0F) {
            return true;
        }
        
        // Keyboard activity
        if (guiIo.WantCaptureKeyboard || guiIo.WantTextInput) {
            return true;
        }
        
        return false;
    }

    void ImGuiFrameRateLimiter::updateActivityState() {
        if (detectActivity()) {
            // Reset counter on activity
            activityCounter_ = decayFrames_;
            currentFps_ = highFps_;
        } else {
            // Decay counter
            if (activityCounter_ > 0) {
                --activityCounter_;
            }
            
            // Switch to low frame rate when counter reaches zero
            if (activityCounter_ == 0) {
                currentFps_ = lowFps_;
            } else {
                currentFps_ = highFps_;
            }
        }
    }

    void ImGuiFrameRateLimiter::waitForNextFrame() {
        updateActivityState();
        
        const double targetFrameTime = 1.0 / currentFps_;
        const double currentTime = glfwGetTime();
        const double deltaTime = currentTime - lastFrameTime_;
        
        if (deltaTime < targetFrameTime) {
            const double sleepTime = targetFrameTime - deltaTime;
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }
        
        lastFrameTime_ = glfwGetTime();
    }

    double ImGuiFrameRateLimiter::getCurrentFps() const {
        return currentFps_;
    }

    bool ImGuiFrameRateLimiter::isHighFrameRate() const {
        return activityCounter_ > 0;
    }

    std::string ImGuiFrameRateLimiter::getModeDescription() const {
        return std::format("Adaptive frame rate: {:.0f}-{:.0f} FPS", lowFps_, highFps_);
    }

} // namespace QaplaWindows
