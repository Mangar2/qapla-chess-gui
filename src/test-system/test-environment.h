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

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE

struct ImGuiTestContext;

#include <string>

/**
 * @file
 * @brief The state every GUI test starts from, set up without touching the GUI.
 *
 * These tests used to start from whatever the developer's own configuration happened to hold --
 * which engines were installed, which time control was last used, how long a snackbar stays up.
 * That is not a starting point, it is a coincidence, and it is why a test could pass here and
 * fail there. Started from an empty configuration directory instead, the coincidence is gone and
 * so is everything the tests were quietly relying on: hence this.
 *
 * Everything here is set directly, on the data. Setting it by clicking would make the setup a
 * test of its own, and a failure in it would look like a failure of whatever came after.
 */

namespace QaplaTest {

/**
 * @brief Brings the application to the state a test may assume. Called first by every test.
 *
 * Idempotent and cheap after the first call: the engines are installed once and stay, the rest
 * is a handful of assignments.
 */
/**
 * @brief Whether a test wants the two playing engines in the catalog, or an empty one.
 *
 * Most tests need engines and should not each set them up. A few are *about* the catalog being
 * empty -- the engine-setup tutorial walks a user from nothing to a working engine -- and for
 * those "the base state" is no engines at all. Either way the state is decided here rather than
 * inherited from whatever was left behind.
 */
enum class TestEngines {
    Installed,
    None
};

void prepareTestEnvironment(ImGuiTestContext* ctx,
    TestEngines engines = TestEngines::Installed);

/**
 * @brief The diagnostic engine the build produced, by variant.
 *
 * @param variant "" for the one that plays, or "noinit", "loop", "lossontime" for the ones that
 *        are broken on purpose. Empty string returned when it is not there.
 */
[[nodiscard]] std::string testEnginePath(const std::string& variant = {});

/**
 * @brief A file the tests read, by name, wherever the source tree happens to be.
 *
 * The tests used to build these paths from the working directory, which meant they only ran when
 * started from the top of the repository -- and gave a file-not-found from anywhere else, at the
 * point of use rather than at the point of the mistake. QAPLA_TEST_DATA_DIR names the directory
 * outright; without it, it is looked for upwards from where the process was started.
 */
[[nodiscard]] std::string testDataPath(const std::string& name);

/**
 * @brief A path for something a test writes, inside the run's own configuration directory.
 *
 * So that a PGN a test produces goes where the rest of the run's leavings go and is thrown away
 * with them, instead of accumulating in the repository.
 */
[[nodiscard]] std::string testOutputPath(const std::string& name);

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
