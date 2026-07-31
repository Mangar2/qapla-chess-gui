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

#include "llm/gui-tool-tournament.h"

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

TEST_CASE("resolveEngines finds configured engines case-insensitively", "[llm][gui-tool-tournament]") {
    EngineConfigGuard guard;
    auto& configs = QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable();
    configs.clear();
    configs.push_back(QaplaTester::EngineConfig::createFromPath("/fake/path/stockfish"));
    configs.push_back(QaplaTester::EngineConfig::createFromPath("/fake/path/qapla"));

    auto outcome = resolveEngines({"STOCKFISH", "qapla"});

    REQUIRE(outcome.resolved.size() == 2);
    REQUIRE(outcome.notFound.empty());
    REQUIRE(outcome.resolved[0].isSelected());
    REQUIRE_FALSE(outcome.resolved[0].isGauntlet());
}

TEST_CASE("resolveEngines reports names with no matching catalog entry", "[llm][gui-tool-tournament]") {
    EngineConfigGuard guard;
    auto& configs = QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable();
    configs.clear();
    configs.push_back(QaplaTester::EngineConfig::createFromPath("/fake/path/stockfish"));

    auto outcome = resolveEngines({"stockfish", "does-not-exist"});

    REQUIRE(outcome.resolved.size() == 1);
    REQUIRE(outcome.notFound == std::vector<std::string>{"does-not-exist"});
}

TEST_CASE("resolveEngines returns nothing for an empty catalog", "[llm][gui-tool-tournament]") {
    EngineConfigGuard guard;
    QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable().clear();

    auto outcome = resolveEngines({"anything"});

    REQUIRE(outcome.resolved.empty());
    REQUIRE(outcome.notFound == std::vector<std::string>{"anything"});
}

TEST_CASE("resolveEngines matches an informal name to the one installed engine that contains it",
    "[llm][gui-tool-tournament]") {
    EngineConfigGuard guard;
    auto& configs = QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable();
    configs.clear();
    auto spike = QaplaTester::EngineConfig::createFromPath("/fake/path/spike");
    spike.setName("Spike 1.4.1");
    configs.push_back(spike);
    auto qapla = QaplaTester::EngineConfig::createFromPath("/fake/path/qapla");
    qapla.setName("Qapla 0.4.0");
    configs.push_back(qapla);

    auto outcome = resolveEngines({"spike"});

    REQUIRE(outcome.resolved.size() == 1);
    REQUIRE(outcome.resolved[0].getName() == "Spike 1.4.1");
    REQUIRE(outcome.notFound.empty());
    REQUIRE(outcome.ambiguous.empty());
}

TEST_CASE("resolveEngines prefers an exact match over a substring match", "[llm][gui-tool-tournament]") {
    EngineConfigGuard guard;
    auto& configs = QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable();
    configs.clear();
    auto spike = QaplaTester::EngineConfig::createFromPath("/fake/path/spike");
    spike.setName("Spike");
    configs.push_back(spike);
    auto spikeVersioned = QaplaTester::EngineConfig::createFromPath("/fake/path/spike2");
    spikeVersioned.setName("Spike 1.4.1");
    configs.push_back(spikeVersioned);

    // "Spike" is an exact match for the first engine -- even though it's also a substring of
    // the second one's name, the exact match must win outright, not be reported ambiguous.
    auto outcome = resolveEngines({"Spike"});

    REQUIRE(outcome.resolved.size() == 1);
    REQUIRE(outcome.resolved[0].getName() == "Spike");
    REQUIRE(outcome.ambiguous.empty());
}

TEST_CASE("resolveEngines reports ambiguous when a name substring-matches more than one installed engine",
    "[llm][gui-tool-tournament]") {
    EngineConfigGuard guard;
    auto& configs = QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable();
    configs.clear();
    auto spike1 = QaplaTester::EngineConfig::createFromPath("/fake/path/spike1");
    spike1.setName("Spike 1.4.1");
    configs.push_back(spike1);
    auto spike2 = QaplaTester::EngineConfig::createFromPath("/fake/path/spike2");
    spike2.setName("Spike Classic");
    configs.push_back(spike2);

    auto outcome = resolveEngines({"spike"});

    REQUIRE(outcome.resolved.empty());
    REQUIRE(outcome.notFound.empty());
    REQUIRE(outcome.ambiguous.size() == 1);
    REQUIRE(outcome.ambiguous[0].given == "spike");
    REQUIRE(outcome.ambiguous[0].matches == std::vector<std::string>{"Spike 1.4.1", "Spike Classic"});
}

TEST_CASE("formatAmbiguousEngineNames lists the candidates for each ambiguous name", "[llm][gui-tool-tournament]") {
    std::vector<AmbiguousEngineName> ambiguous{
        {.given = "spike", .matches = {"Spike 1.4.1", "Spike Classic"}}
    };

    auto message = formatAmbiguousEngineNames(ambiguous);

    REQUIRE(message == "\"spike\" could mean: Spike 1.4.1, Spike Classic.");
}
