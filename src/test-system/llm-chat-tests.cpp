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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "test-environment.h"
#include "test-system/llm-chat-tests.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui.h>
#include "imgui_te_engine.h"
#include "imgui_te_context.h"
#include "chatbot/chatbot-window.h"
#include "chatbot/chatbot-llm-chat.h"
#include "llm/lm-studio-locator.h"

namespace QaplaTest {

    namespace {
        void resetChatbotToInitialState(ImGuiTestContext* ctx) {
            // Also strips any AI-Chat thread a real LM Studio detection may
            // already have registered, giving each test a clean baseline.
            QaplaWindows::ChatBot::ChatbotWindow::instance()->reset();
            ctx->Yield();
        }
    }

    void registerLlmChatTests(ImGuiTestEngine* engine) {
        ImGuiTest* t = nullptr;

        // -----------------------------------------------------------------
        // Test 1: AI Chat entry is absent until LM Studio is registered
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "LlmChat/Registration", "AbsentByDefault");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            prepareTestEnvironment(ctx);
            ctx->LogInfo("=== Test: AI Chat entry absent by default ===");

            resetChatbotToInitialState(ctx);

            // The tab is "###Chatbot"; the ref used to carry a label in front of it that no
            // widget has, so the click never landed -- and the check after it was
            // reading a window nobody had switched to.
            ctx->ItemClick("**/###Chatbot");
            ctx->Yield(2);

            IM_CHECK(!ctx->ItemExists("**/###AI Chat"));

            ctx->LogInfo("=== Test AbsentByDefault PASSED ===");
        };

        // -----------------------------------------------------------------
        // Test 2: AI Chat entry appears once LM Studio was detected
        // -----------------------------------------------------------------
        t = IM_REGISTER_TEST(engine, "LlmChat/Registration", "AppearsWhenDetected");
        t->TestFunc = [](ImGuiTestContext* ctx) {
            prepareTestEnvironment(ctx);
            ctx->LogInfo("=== Test: AI Chat entry appears after detection ===");

            resetChatbotToInitialState(ctx);

            // Simulates what QaplaLlm::startLlmChatDetection() does once the
            // async LmStudioLocator probe finds a server or installation.
            QaplaWindows::ChatBot::ChatbotWindow::instance()->registerThread(
                std::make_unique<QaplaWindows::ChatBot::ChatbotLlmChat>(
                    QaplaLlm::LmStudioStatus::ServerRunning));
            ctx->Yield();

            // The tab is "###Chatbot"; the ref used to carry a label in front of it that no
            // widget has, so the click never landed -- and the check after it was
            // reading a window nobody had switched to.
            ctx->ItemClick("**/###Chatbot");
            ctx->Yield(2);

            IM_CHECK(ctx->ItemExists("**/###AI Chat"));

            ctx->LogInfo("Step: Opening AI Chat...");
            ctx->ItemClick("**/###AI Chat");
            ctx->Yield(2);

            IM_CHECK(ctx->ItemExists("**/##LlmChatInput"));

            ctx->LogInfo("Step: Closing AI Chat...");
            ctx->ItemClick("**/###Close");
            ctx->Yield(2);

            ctx->LogInfo("=== Test AppearsWhenDetected PASSED ===");
        };
    }

} // namespace QaplaTest
#endif
