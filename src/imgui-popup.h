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
#include "imgui-controls.h"

#include <imgui.h>

#include <memory>
#include <functional>
#include <optional>
#include <string>

namespace QaplaWindows {
    template<typename T>
    concept EmbeddedWindowType = std::derived_from<T, EmbeddedWindow>;
    
    /**
     * @brief A reusable popup window that displays any EmbeddedWindow-derived content
     * with standard buttons like OK and Cancel.
     */
    template<EmbeddedWindowType T>
    class ImGuiPopup {
    public:
        struct Config {
			std::string title;
			bool okButton = true;
			bool cancelButton = true;
        };

        /**
         * @brief Creates a popup window.
         * @param config configuration
         */
        ImGuiPopup(Config config, auto size = ImVec2(400, 300))
            : content_(), config_(config), size_(size) {
        }

        /**
         * @brief Renders the popup. Should be called every frame.
         */
        void draw(const std::string& ok = "OK", const std::string& cancel = "Cancel") {
            if (!isOpen_) return;
            ImGui::SetNextWindowSize(size_, ImGuiCond_Once);
            if (!ImGui::BeginPopupModal(config_.title.c_str(), nullptr, ImGuiWindowFlags_None))
                return;

            ImGui::BeginChild("popup_content", ImVec2(0, -50), false);
            content_.draw();
            ImGui::EndChild();

            ImGui::Separator();
            const auto buttonSize = ImVec2(80.0F, 25.0F);
			const float buttonSpacing = 30.0F;

			ImGui::SetCursorPosX(buttonSpacing);
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - buttonSize.y - 8.0F);
            if (config_.okButton) {
                if (ImGuiControls::textButton(ok.c_str(), buttonSize)) {
                    confirmed_ = true;
                    close();
                }
                ImGui::SameLine();
            }

			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonSize.x - buttonSpacing);
            if (config_.cancelButton) {
                if (ImGuiControls::textButton(cancel.c_str(), buttonSize)) {
                    confirmed_ = false;
                    close();
                }
            }

            ImGui::EndPopup();
        }

        /**
         * @brief Returns whether the popup was confirmed (OK clicked).
         * @return std::optional<bool>: true if OK, false if Cancel, nullopt if still open.
         */
        std::optional<bool> confirmed() const {
            return confirmed_;
        }

        /**
         * @brief Resets the confirmation state to std::nullopt to avoid reprocessing a previous result.
         */
        void resetConfirmation() {
			confirmed_ = std::nullopt;
		}

        /**
         * @brief Opens the popup manually.
         */
        void open() {
            isOpen_ = true;
            confirmed_ = std::nullopt;
            ImGui::OpenPopup(config_.title.c_str());
        }

        /**
		 * @brief Returns the content window of this popup.
         */
        const T& content() const {
            return content_;
		}

        T& content() {
            return content_;
        }

    private:

        void close() {
            isOpen_ = false;
            ImGui::CloseCurrentPopup();
		}

        T content_;
        Config config_;
        ImVec2 size_;
        std::optional<bool> confirmed_ = std::nullopt;
		bool isOpen_ = false;
    };

} // namespace QaplaWindows