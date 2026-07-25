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

#include <catch2/catch_test_macros.hpp>

#include "llm/gui-tool-engine-management.h"

#include <engine-handling/engine-worker-factory.h>

using namespace QaplaLlm;

namespace {
    // The engine catalog is a process-wide singleton; save/restore it so
    // this test doesn't leak fabricated engines into other tests.
    struct EngineConfigGuard {
        std::vector<QaplaTester::EngineConfig> saved =
            QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigs();

        ~EngineConfigGuard() {
            QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable() = saved;
        }
    };
}

TEST_CASE("addEnginesFromPaths adds new engines and skips duplicates", "[llm][gui-tool-engine-management]") {
    EngineConfigGuard guard;
    QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable().clear();

    auto outcome = addEnginesFromPaths({"/fake/path/engineA.exe", "/fake/path/engineB.exe"});
    REQUIRE(outcome.addedNames.size() == 2);
    REQUIRE(outcome.duplicateNames.empty());

    auto secondOutcome = addEnginesFromPaths({"/fake/path/engineA.exe"});
    REQUIRE(secondOutcome.addedNames.empty());
    REQUIRE(secondOutcome.duplicateNames.size() == 1);

    REQUIRE(QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigs().size() == 2);
}

TEST_CASE("listInstalledEnginesJson reflects the configured engines", "[llm][gui-tool-engine-management]") {
    EngineConfigGuard guard;
    QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable().clear();

    auto addOutcome = addEnginesFromPaths({"/fake/path/stockfish.exe"});
    REQUIRE(addOutcome.addedNames.size() == 1);

    auto json = listInstalledEnginesJson();
    REQUIRE(json.find("stockfish") != std::string::npos);
    REQUIRE(json.find("uci") != std::string::npos);
}

TEST_CASE("listInstalledEnginesJson returns an empty array when nothing is configured",
    "[llm][gui-tool-engine-management]") {
    EngineConfigGuard guard;
    QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable().clear();

    REQUIRE(listInstalledEnginesJson() == "[]");
}

// registerEngineManagementTools() itself (gui-tool-engine-management-register.cpp)
// is not covered here: it wires up OsDialogs + Configuration, which
// transitively pull in the ImGui/GLFW GUI stack that the unit-tests target
// deliberately does not link against (same reasoning as LmStudioLocator's
// real-filesystem/real-server paths). Its handlers are thin wrappers around
// the pure functions tested above; the file dialog itself can only be
// exercised manually or via an imgui_test_engine GUI test.
