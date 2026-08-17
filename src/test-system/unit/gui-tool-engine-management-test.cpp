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

    using QaplaTester::EngineOption;

    constexpr const char* ENGINE_PATH = "/fake/path/tuned.exe";

    /** @brief An engine configuration for ENGINE_PATH, without touching the catalog singleton. */
    QaplaTester::EngineConfig makeConfig() {
        return QaplaTester::EngineConfig::createFromPath(ENGINE_PATH);
    }

    /**
     * @brief Fills a capability set as detection would have left it: keyed by path+protocol.
     *
     * That keying is the point of the fixture -- it is what lets two differently named
     * configurations of one build share an option list, which is the whole basis of testing the
     * same engine against itself under two option sets.
     *
     * Filled in place rather than returned, because EngineCapabilities holds a detection flag and
     * is therefore non-copyable.
     */
    void fillCapabilities(QaplaConfiguration::EngineCapabilities& capabilities) {
        QaplaConfiguration::EngineCapability capability;
        capability.setPath(ENGINE_PATH);
        capability.setProtocol(QaplaTester::EngineProtocol::Uci);
        capability.setSupportedOptions(QaplaTester::EngineOptions{
            EngineOption{.name = "Hash", .type = EngineOption::Type::Spin,
                .defaultValue = "16", .min = 1, .max = 65536, .vars = {}},
            EngineOption{.name = "Ponder", .type = EngineOption::Type::Check,
                .defaultValue = "false", .min = {}, .max = {}, .vars = {}},
            EngineOption{.name = "Style", .type = EngineOption::Type::Combo,
                .defaultValue = "normal", .min = {}, .max = {},
                .vars = {"normal", "aggressive"}},
            EngineOption{.name = "Clear Hash", .type = EngineOption::Type::Button,
                .defaultValue = "", .min = {}, .max = {}, .vars = {}}});
        capabilities.addOrReplace(capability);
    }

    /** @brief A filled capability set, declared in one line at the top of a test. */
    struct DetectedEngine {
        QaplaConfiguration::EngineCapabilities capabilities;
        DetectedEngine() { fillCapabilities(capabilities); }
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

TEST_CASE("addNamedEngines installs one executable twice under two names",
    "[llm][gui-tool-engine-management]") {
    EngineConfigGuard guard;
    QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable().clear();

    // The tuning case: two catalog entries on one build, so each can carry its own option values.
    // Deduplicating by executable -- right for the file dialog, where the name is only a filename
    // -- would make this impossible to express at all.
    auto outcome = addNamedEngines({{.name = "baseline", .path = ENGINE_PATH},
        {.name = "candidate", .path = ENGINE_PATH}});

    REQUIRE(outcome.addedNames.size() == 2);
    REQUIRE(outcome.takenNames.empty());
    REQUIRE(QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs().size() == 2);
}

TEST_CASE("addNamedEngines refuses a name that is already taken",
    "[llm][gui-tool-engine-management]") {
    EngineConfigGuard guard;
    QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable().clear();

    REQUIRE(addNamedEngines({{.name = "baseline", .path = ENGINE_PATH}}).addedNames.size() == 1);
    auto outcome = addNamedEngines({{.name = "baseline", .path = "/fake/path/other.exe"}});

    // The name is the key every other tool refers to the engine by, so a second entry under it
    // would leave neither reachable.
    REQUIRE(outcome.addedNames.empty());
    REQUIRE(outcome.takenNames.size() == 1);
    REQUIRE(QaplaTester::EngineWorkerFactory::getConfigManager().getAllConfigs().size() == 1);
}

TEST_CASE("addNamedEngines keeps the chosen name out of detection's reach",
    "[llm][gui-tool-engine-management]") {
    EngineConfigGuard guard;
    QaplaTester::EngineWorkerFactory::getConfigManagerMutable().getAllConfigsMutable().clear();

    REQUIRE(addNamedEngines({{.name = "candidate", .path = ENGINE_PATH}}).addedNames.size() == 1);

    // Detection replaces a name that is still the executable's filename with the one the engine
    // reports (EngineCapabilities::storeCapabilities). A chosen name must not qualify, or two
    // builds of one engine both come back named after the engine.
    const auto* config = QaplaTester::EngineWorkerFactory::getConfigManager().getConfig("candidate");
    REQUIRE(config != nullptr);
    REQUIRE_FALSE(config->hasDefaultName());
}

TEST_CASE("applyEngineOptions stores a value the engine actually offers",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const DetectedEngine engine;
    const auto& capabilities = engine.capabilities;

    auto outcome = applyEngineOptions(config, {{.name = "Hash", .value = "256"}}, capabilities);

    REQUIRE(outcome.detected);
    REQUIRE(outcome.applied.size() == 1);
    REQUIRE(outcome.unknown.empty());
    REQUIRE(outcome.rejected.empty());
    REQUIRE(config.getOptionValues().at("Hash") == "256");
}

TEST_CASE("applyEngineOptions stores an option under the engine's own spelling",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const DetectedEngine engine;
    const auto& capabilities = engine.capabilities;

    // What reaches the engine has to be what the engine called it, however the caller typed it.
    auto outcome = applyEngineOptions(config, {{.name = "hash", .value = "64"}}, capabilities);

    REQUIRE(outcome.applied.size() == 1);
    REQUIRE(config.getOptionValues().contains("Hash"));
    REQUIRE_FALSE(config.getOptionValues().contains("hash"));
}

TEST_CASE("applyEngineOptions refuses an option this engine does not have",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const DetectedEngine engine;
    const auto& capabilities = engine.capabilities;

    // The failure this guards against is silent: an engine ignores what it does not understand,
    // so a misspelt name would otherwise produce a run that completed and measured the wrong thing.
    auto outcome = applyEngineOptions(config, {{.name = "Hasch", .value = "256"}}, capabilities);

    REQUIRE(outcome.unknown.size() == 1);
    REQUIRE(outcome.applied.empty());
    REQUIRE(config.getOptionValues().empty());
}

TEST_CASE("applyEngineOptions refuses values outside the option's declared domain",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const DetectedEngine engine;
    const auto& capabilities = engine.capabilities;

    auto outcome = applyEngineOptions(config,
        {{.name = "Hash", .value = "99999999"}, {.name = "Ponder", .value = "yes"},
            {.name = "Style", .value = "wild"}, {.name = "Clear Hash", .value = "1"}},
        capabilities);

    REQUIRE(outcome.rejected.size() == 4);
    REQUIRE(outcome.applied.empty());
    // Each rejection has to name the domain it violated, or the caller needs a second lookup to
    // find out what would have been allowed.
    REQUIRE(outcome.rejected[0].find("65536") != std::string::npos);
    REQUIRE(outcome.rejected[1].find("true or false") != std::string::npos);
    REQUIRE(outcome.rejected[2].find("aggressive") != std::string::npos);
    REQUIRE(outcome.rejected[3].find("no value") != std::string::npos);
}

TEST_CASE("applyEngineOptions applies the good values out of a mixed set",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const DetectedEngine engine;
    const auto& capabilities = engine.capabilities;

    // One bad entry must not cost the caller the whole call -- the same asymmetry the argument
    // mapper uses for optional parameters.
    auto outcome = applyEngineOptions(config,
        {{.name = "Hash", .value = "128"}, {.name = "Nonsense", .value = "1"},
            {.name = "Style", .value = "aggressive"}},
        capabilities);

    REQUIRE(outcome.applied.size() == 2);
    REQUIRE(outcome.unknown.size() == 1);
    REQUIRE(config.getOptionValues().at("Hash") == "128");
    REQUIRE(config.getOptionValues().at("Style") == "aggressive");
}

TEST_CASE("applyEngineOptions tells an undetected engine from an unknown option",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const QaplaConfiguration::EngineCapabilities empty;

    auto outcome = applyEngineOptions(config, {{.name = "Hash", .value = "256"}}, empty);

    // Two different answers: here the question could not be asked at all, so reporting "Hash does
    // not exist" would send the caller looking for a different option name that also cannot work.
    REQUIRE_FALSE(outcome.detected);
    REQUIRE(outcome.unknown.empty());
    REQUIRE(config.getOptionValues().empty());
}

TEST_CASE("applyEngineOptions keeps two configurations of one build independent",
    "[llm][gui-tool-engine-management]") {
    auto baseline = makeConfig();
    auto candidate = makeConfig();
    const DetectedEngine engine;
    const auto& capabilities = engine.capabilities;

    // The case the whole design serves: one executable, one capability list, two option sets.
    REQUIRE(applyEngineOptions(baseline, {{.name = "Hash", .value = "16"}}, capabilities).detected);
    REQUIRE(applyEngineOptions(candidate, {{.name = "Hash", .value = "512"}}, capabilities).detected);

    REQUIRE(baseline.getOptionValues().at("Hash") == "16");
    REQUIRE(candidate.getOptionValues().at("Hash") == "512");
}

TEST_CASE("engineDetailsText reports the domain, not just the option names",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const DetectedEngine engine;
    const auto& capabilities = engine.capabilities;
    config.setOptionValue("Hash", "256");

    const auto text = engineDetailsText(config, capabilities);

    REQUIRE(text.find("Hash = 256") != std::string::npos);       // what is set
    REQUIRE(text.find("1..65536") != std::string::npos);         // what may be set
    REQUIRE(text.find("default 16") != std::string::npos);
    REQUIRE(text.find("aggressive") != std::string::npos);       // the choices of a combo
    REQUIRE(text.find(ENGINE_PATH) != std::string::npos);
}

TEST_CASE("engineDetailsText says so when the engine never reported anything",
    "[llm][gui-tool-engine-management]") {
    auto config = makeConfig();
    const QaplaConfiguration::EngineCapabilities empty;

    const auto text = engineDetailsText(config, empty);

    REQUIRE(text.find("unknown") != std::string::npos);
}

// The engine actions themselves (src/llm/actions/gui-action-engines.cpp) are not covered here:
// they wire up OsDialogs + Configuration, which transitively pull in the ImGui/GLFW GUI stack
// that the unit-tests target deliberately does not link against (same reasoning as
// LmStudioLocator's real-filesystem/real-server paths). They are thin wrappers around the pure
// functions tested above; the file dialog itself can only be exercised manually or via an
// imgui_test_engine GUI test. The tool declarations that call them go through the mapper, which
// is covered on its own in llm-tool-api-test.cpp.
