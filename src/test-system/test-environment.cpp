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

#ifdef IMGUI_ENABLE_TEST_ENGINE

#include "imgui_te_context.h"

#include "chatbot/chatbot-window.h"
#include "configuration.h"
#include "llm/gui-tool-engine-management.h"
#include "os-helpers.h"
#include "snackbar.h"

#include <engine-handling/engine-worker-factory.h>

#include <filesystem>
#include <string>
#include <vector>

namespace QaplaTest {

namespace {

namespace fs = std::filesystem;

/** @brief The name a test engine is installed under. Two, because a tournament needs opponents. */
constexpr const char* FIRST_ENGINE = "Test Engine A";
constexpr const char* SECOND_ENGINE = "Test Engine B";

[[nodiscard]] std::string executableSuffix() {
#ifdef _WIN32
    return ".exe";
#else
    return {};
#endif
}

/**
 * @brief Finds the diagnostic engine the build produced.
 *
 * QAPLA_TEST_ENGINE_DIR names it outright, which is what a test runner sets. Without it, the
 * usual build directories are tried, so starting the tests by hand from the source tree works
 * too. The diagnostic engine is used rather than a real one because it is built alongside the
 * GUI: a test that needs an engine installed should not also need somebody to have put one
 * somewhere first.
 */
[[nodiscard]] fs::path findDiagnosticEngine(const std::string& variant) {
    const std::string name = variant.empty()
        ? "diagnostic-engine" + executableSuffix()
        : "diagnostic-engine-" + variant + executableSuffix();

    if (const auto configured = QaplaHelpers::OsHelpers::getEnv("QAPLA_TEST_ENGINE_DIR")) {
        const fs::path candidate = fs::path(*configured) / name;
        if (fs::is_regular_file(candidate)) {
            return candidate;
        }
    }

    std::error_code error;
    for (const auto* preset : {"default", "test", "releasetest", "release"}) {
        for (const auto& root : {fs::current_path(error), fs::path("..")}) {
            const fs::path candidate = root / "build" / preset / "bin" / name;
            if (fs::is_regular_file(candidate)) {
                return candidate;
            }
        }
    }
    return {};
}

/**
 * @brief Makes a second engine out of the first, so a tournament has two sides.
 *
 * The build produces one playing diagnostic engine (the other variants are deliberately broken).
 * The GUI tells engines apart by their executable, so a second opponent has to be a second file.
 * It is put in the configuration directory, which for a test run is thrown away with everything
 * else. The copy carries no mode word in its name, so it plays like the original.
 */
[[nodiscard]] fs::path secondEngineFrom(const fs::path& source) {
    const fs::path target = fs::path(QaplaHelpers::OsHelpers::getConfigDirectory())
        / ("test-engine-b" + executableSuffix());
    std::error_code error;
    if (!fs::is_regular_file(target)) {
        fs::create_directories(target.parent_path(), error);
        fs::copy_file(source, target, fs::copy_options::overwrite_existing, error);
        if (error) {
            return {};
        }
        fs::permissions(target, fs::perms::owner_all | fs::perms::group_exec | fs::perms::others_exec,
            fs::perm_options::add, error);
    }
    return target;
}

void selectAllEngines();

/** @brief Whether the catalog already holds what the tests need. */
[[nodiscard]] bool enginesAreInstalled() {
    return QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs().size() >= 2;
}

/**
 * @brief Puts two playing engines into the catalog, and waits for them to be detected.
 *
 * Waited for by letting frames run, not by blocking: detection reports what it finds through the
 * frame loop (see UiUpdateQueue), so a test that stopped the loop to wait would wait for itself.
 */
void installTestEngines(ImGuiTestContext* ctx) {
    if (enginesAreInstalled()) {
        return;
    }

    const fs::path first = findDiagnosticEngine({});
    if (first.empty()) {
        IM_CHECK_SILENT(!"the diagnostic engine was not found -- build the project, or set "
                         "QAPLA_TEST_ENGINE_DIR to the directory holding it");
        return;
    }
    const fs::path second = secondEngineFrom(first);
    IM_CHECK_SILENT(!second.empty());

    std::vector<QaplaLlm::NamedEnginePath> engines{
        {.name = FIRST_ENGINE, .path = first.string()},
        {.name = SECOND_ENGINE, .path = second.string()}};
    static_cast<void>(QaplaLlm::addNamedEngines(engines));

    auto& capabilities = QaplaConfiguration::Configuration::instance().getEngineCapabilities();
    capabilities.autoDetect();
    for (int frame = 0; frame < 2000 && capabilities.isDetecting(); ++frame) {
        ctx->Yield();
    }

    IM_CHECK_SILENT(enginesAreInstalled());
    selectAllEngines();
}

/**
 * @brief Marks the installed engines as selected, in the catalog.
 *
 * Selection lives on the engine configuration itself, so doing it here means every view that
 * takes its list from the catalog -- a tournament, an EPD run, a board opened later -- starts
 * with engines chosen. A test that wants a particular selection still sets its own; what this
 * removes is the tests that quietly relied on the developer's own configuration having something
 * selected already, and did nothing at all when it did not.
 */
void selectAllEngines() {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    for (const auto& config : configManager.getAllConfigs()) {
        auto* stored = configManager.getConfigMutableByCmdAndProtocol(
            config.getCmd(), config.getProtocol());
        if (stored != nullptr) {
            stored->setSelected(true);
        }
    }
}

/**
 * @brief Takes the snackbar out of the way.
 *
 * It is a notification that sits over the window for ten to twenty seconds, and a test driving
 * the GUI has to click where it is. One second is long enough to still see one in a recording
 * and short enough that the next step is not fighting it.
 */
void shortenSnackbarMessages() {
    QaplaWindows::SnackbarManager::SnackbarConfig config;
    config.noteDurationInS = 1;
    config.successDurationInS = 1;
    config.warningDurationInS = 1;
    config.errorDurationInS = 1;
    QaplaWindows::SnackbarManager::instance().setConfig(config);
}

} // namespace

std::string testDataPath(const std::string& name) {
    static const fs::path directory = []() -> fs::path {
        if (const auto configured = QaplaHelpers::OsHelpers::getEnv("QAPLA_TEST_DATA_DIR")) {
            return fs::path(*configured);
        }
        // Upwards from wherever this was started: the tests may be run from the build directory,
        // from the source root, or from a directory made for the run.
        std::error_code error;
        fs::path here = fs::current_path(error);
        for (int level = 0; level < 6 && !here.empty(); ++level) {
            const fs::path candidate = here / "src" / "test-system" / "test-data";
            if (fs::is_directory(candidate)) {
                return candidate;
            }
            if (!here.has_parent_path() || here.parent_path() == here) {
                break;
            }
            here = here.parent_path();
        }
        return {};
    }();

    return directory.empty() ? name : (directory / name).string();
}

std::string testOutputPath(const std::string& name) {
    return (fs::path(QaplaHelpers::OsHelpers::getConfigDirectory()) / name).string();
}

/** @brief Empties the catalog, for the tests that are about it being empty. */
void removeAllEngines() {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    for (const auto& config : configManager.getAllConfigs()) {
        configManager.removeConfig(config);
    }
}


std::string testEnginePath(const std::string& variant) {
    return findDiagnosticEngine(variant).string();
}

void prepareTestEnvironment(ImGuiTestContext* ctx, TestEngines engines) {
    shortenSnackbarMessages();
    QaplaWindows::SnackbarManager::instance().dismissAll();
    if (engines == TestEngines::Installed) {
        installTestEngines(ctx);
    } else {
        removeAllEngines();
    }
    QaplaWindows::ChatBot::ChatbotWindow::instance()->reset();
    ctx->Yield();
}

} // namespace QaplaTest

#endif // IMGUI_ENABLE_TEST_ENGINE
