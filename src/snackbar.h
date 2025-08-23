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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */

#pragma once

#include "imgui.h"

#include <string>
#include <chrono>
#include <optional>
#include <array>
#include <deque> // F�r den Stack von Snackbar-Eintr�gen

class SnackbarManager {
public:
    enum class SnackbarType {
        Note,
        Success,
        Warning,
        Error,
        Count
    };

    void showError(const std::string& message) {
        show(message, SnackbarType::Error);
    }
    void showWarning(const std::string& message) {
        show(message, SnackbarType::Warning);
    }
    void showSuccess(const std::string& message) {
        show(message, SnackbarType::Success);
    }
    void showNote(const std::string& message) {
        show(message, SnackbarType::Note);
    }

    void show(const std::string& message, SnackbarType type) {
        SnackbarEntry entry;
        if (snackbarStack_.size() >0 && snackbarStack_.back().message == message && snackbarStack_.back().type == type) {
            snackbarStack_.back().startTime = std::chrono::steady_clock::now();
            return;
		}
        entry.message = message;
        if (entry.message[0] == '\n') {
            entry.message.erase(0, 1); 
		}
        entry.startTime = std::chrono::steady_clock::now();
        entry.duration = durations[static_cast<int>(type)];
		entry.type = type;
        snackbarStack_.emplace_back(std::move(entry)); 
    }

    static SnackbarManager& instance() {
        static SnackbarManager instance;
        return instance;
    }

    void draw() {
        constexpr ImVec2 snackbarSize = ImVec2(450.0f, 120.0f); 
        constexpr float closeButtonRadius = 10.0f;

        while (!snackbarStack_.empty()) {
            auto& currentSnackbar = snackbarStack_.back();
            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(now - currentSnackbar.startTime).count();

            if (elapsed > currentSnackbar.duration) {
                snackbarStack_.pop_back();
                continue;
            }

            ImGui::PushStyleColor(ImGuiCol_WindowBg, colors[static_cast<int>(currentSnackbar.type)]);

            ImVec2 windowPos = ImGui::GetMainViewport()->Pos;
            ImVec2 windowSize = ImGui::GetMainViewport()->Size;
            ImVec2 snackbarPos = ImVec2(windowPos.x + 20.0f, windowPos.y + windowSize.y - snackbarSize.y - 20.0f);

            ImGui::SetNextWindowPos(snackbarPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(snackbarSize, ImGuiCond_Always); 

            ImGui::Begin("##Snackbar", nullptr, ImGuiWindowFlags_NoDecoration);
            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x); 
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::SetWindowFontScale(1.1f);
			ImGui::SetCursorPos(ImVec2(0.0f, 20.0f)); // Position des Textes
			ImGui::Indent(20.0f); 
			ImGui::Text("%s:", typeNames[static_cast<int>(currentSnackbar.type)]);
            ImGui::Text("%s", currentSnackbar.message.c_str());
			ImGui::Unindent(20.0f);
            ImGui::SetWindowFontScale(1.0f);
			ImGui::PopStyleColor();
            ImGui::PopTextWrapPos(); 

            ImVec2 closeButtonPos = ImVec2(
                snackbarPos.x + snackbarSize.x - closeButtonRadius - 10.0f, 
                snackbarPos.y + closeButtonRadius + 10.0f                  
            );
            if (drawCloseButton(closeButtonPos, closeButtonRadius)) {
                snackbarStack_.pop_back(); 
            }

            ImGui::End();
            ImGui::PopStyleColor();

            break; 
        }
    }

private:
    struct SnackbarEntry {
        std::string message;
        std::chrono::steady_clock::time_point startTime;
		SnackbarType type;
        float duration;
    };

    static constexpr std::array<ImVec4, static_cast<size_t>(SnackbarType::Count)> colors = {
        ImVec4(0.8f, 0.8f, 0.8f, 1.0f), // Note: Blass-Grau
        ImVec4(0.7f, 0.8f, 0.7f, 1.0f), // Success: Blass-Gr�n
        ImVec4(0.8f, 0.8f, 0.7f, 1.0f), // Warning: Blass-Gelb
        ImVec4(0.8f, 0.7f, 0.7f, 1.0f)  // Error: Blass-Rot
    };

    static constexpr std::array<float, static_cast<size_t>(SnackbarType::Count)> durations = {
        10.0f, // Note
        10.0f, // Success
        15.0f, // Warning
        20.0f  // Error
    };

    static constexpr std::array<const char*, static_cast<size_t>(SnackbarType::Count)> typeNames = {
        "Note",    
        "Success", 
        "Warning", 
        "Error"    
	};

    bool drawCloseButton(const ImVec2& position, float radius) {
        auto drawList = ImGui::GetWindowDrawList();

        // Circle
        drawList->AddCircleFilled(position, radius, ImColor(1.0f, 1.0f, 1.0f, 0.9f)); // Wei�er Kreis
        drawList->AddCircle(position, radius, ImColor(0.0f, 0.0f, 0.0f, 0.9f), 16, 1.5f); // Schwarzer Rand

        // "X" 
        float lineThickness = 2.0f;
        float crossSize = radius * 0.5f;
        ImVec2 lineStart1 = ImVec2(position.x - crossSize, position.y - crossSize);
        ImVec2 lineEnd1 = ImVec2(position.x + crossSize, position.y + crossSize);
        ImVec2 lineStart2 = ImVec2(position.x - crossSize, position.y + crossSize);
        ImVec2 lineEnd2 = ImVec2(position.x + crossSize, position.y - crossSize);

        drawList->AddLine(lineStart1, lineEnd1, ImColor(0.0f, 0.0f, 0.0f, 0.9f), lineThickness); // Erste Linie des "X"
        drawList->AddLine(lineStart2, lineEnd2, ImColor(0.0f, 0.0f, 0.0f, 0.9f), lineThickness); // Zweite Linie des "X"

        ImGui::SetCursorScreenPos(ImVec2(position.x - radius, position.y - radius));
        ImGui::InvisibleButton("CloseButton", ImVec2(radius * 2, radius * 2));
        return ImGui::IsItemClicked(); // Gibt true zur�ck, wenn der Button geklickt wurde
    }

    std::deque<SnackbarEntry> snackbarStack_; // Stack f�r Snackbar-Eintr�ge
};