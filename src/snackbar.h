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

#include "imgui.h"

#include <string>
#include <chrono>
#include <optional>



class SnackbarManager {
public:
    void showSnackbar(const std::string& message, float duration = 3.0f) {
        snackbarMessage_ = message;
        snackbarStartTime_ = std::chrono::steady_clock::now();
        snackbarDuration_ = duration;
    }

    static SnackbarManager& instance() {
        static SnackbarManager instance;
        return instance;
	}

    void draw() {
        if (!snackbarMessage_.has_value()) return;

        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - snackbarStartTime_).count();

        if (elapsed > snackbarDuration_) {
            snackbarMessage_.reset(); // Nachricht entfernen, wenn die Zeit abgelaufen ist
            return;
        }

        // Snackbar-Position und Stil
        ImVec2 windowPos = ImGui::GetMainViewport()->Pos;
        ImVec2 windowSize = ImGui::GetMainViewport()->Size;
        ImVec2 snackbarPos = ImVec2(windowPos.x + 20.0f, windowPos.y + windowSize.y - 60.0f);

        ImGui::SetNextWindowPos(snackbarPos, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.9f); // Transparenter Hintergrund
        ImGui::Begin("##Snackbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
        ImGui::Text("%s", snackbarMessage_->c_str());
        ImGui::End();
    }

private:
    std::optional<std::string> snackbarMessage_;
    std::chrono::steady_clock::time_point snackbarStartTime_;
    float snackbarDuration_ = 0.0f;
};