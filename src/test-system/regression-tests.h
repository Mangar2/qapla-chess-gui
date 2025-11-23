/**
 * @license
 * ... (License header) ...
 */

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE
struct ImGuiTestEngine;

namespace QaplaTest {

    /**
     * @brief Registers all regression tests with the engine.
     * @param engine The test engine instance.
     */
    void registerRegressionTests(ImGuiTestEngine* engine);

} // namespace QaplaTest
#endif
