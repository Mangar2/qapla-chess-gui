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
 * @author GitHub Copilot
 * @copyright Copyright (c) 2025 GitHub Copilot
 */

#include "chatbot-choose-language.h"

void ChatbotChooseLanguage::start() {
    m_finished = true;
}

void ChatbotChooseLanguage::draw() {
    // Intentionally empty
}

bool ChatbotChooseLanguage::isFinished() const {
    return m_finished;
}

std::unique_ptr<ChatbotThread> ChatbotChooseLanguage::clone() const {
    return std::make_unique<ChatbotChooseLanguage>();
}
