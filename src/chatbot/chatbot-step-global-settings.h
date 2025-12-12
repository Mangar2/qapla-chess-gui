/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#pragma once

#include "chatbot-step.h"
#include <functional>

namespace QaplaWindows {
    class ImGuiEngineGlobalSettings;
}

namespace QaplaWindows::ChatBot {

/**
 * @brief Step to configure global engine settings (hash, time control).
 * Supports tournaments, SPRT, EPD, and interactive boards via callback.
 */
class ChatbotStepGlobalSettings : public ChatbotStep {
public:
    /**
     * @brief Callback function type that returns a reference to global settings.
     * The callback may return nullptr if the target object no longer exists.
     */
    using SettingsProvider = std::function<ImGuiEngineGlobalSettings*()>;

    /**
     * @brief Constructs with a settings provider callback.
     */
    explicit ChatbotStepGlobalSettings(SettingsProvider provider);
    
    ~ChatbotStepGlobalSettings() override = default;

    [[nodiscard]] std::string draw() override;

private:
    SettingsProvider provider_;        ///< Callback for settings
    bool showMoreOptions_ = false;     ///< Show advanced options

    /**
     * @brief Gets the global settings from the provider.
     * @return Pointer to global settings, or nullptr if target no longer exists.
     */
    [[nodiscard]] ImGuiEngineGlobalSettings* getGlobalSettings();
};

} // namespace QaplaWindows::ChatBot
