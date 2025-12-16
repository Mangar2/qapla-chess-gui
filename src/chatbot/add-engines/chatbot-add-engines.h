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

#include "../chatbot-thread.h"
#include "../chatbot-step.h"
#include "imgui-engine-select.h"
#include <vector>
#include <memory>

namespace QaplaWindows::ChatBot {

/**
 * @brief A chatbot thread for adding new chess engines to the engine list.
 * 
 * This thread guides users through adding engine binaries to the global engine list.
 * Each engine can only be added once (file path must be unique), but can be used
 * multiple times with different configurations in tournaments, EPD analysis, etc.
 */
class ChatbotAddEngines : public ChatbotThread {
public:
    ChatbotAddEngines();
    
    [[nodiscard]] std::string getTitle() const override { 
        return "Add Engines"; 
    }
    void start() override;
    bool draw() override;
    [[nodiscard]] bool isFinished() const override;
    [[nodiscard]] std::unique_ptr<ChatbotThread> clone() const override;

private:
    std::vector<std::unique_ptr<ChatbotStep>> steps_;
    size_t currentStepIndex_ = 0;
    bool stopped_ = false;
    std::unique_ptr<QaplaWindows::ImGuiEngineSelect> engineSelect_;
    
    void addInitialSteps();
};

} // namespace QaplaWindows::ChatBot
