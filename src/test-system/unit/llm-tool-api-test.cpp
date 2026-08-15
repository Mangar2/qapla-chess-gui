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

// Tests the mapper between the external (model-facing) tool API and the internal action API --
// the piece that makes a parameter's declaration its schema AND its reader, so the two cannot
// drift apart. The tools and actions themselves are GUI-bound and covered by the ImGui test suite
// (src/test-system/llm-*-tool-tests.cpp); what matters here is the generic machinery.

#include <catch2/catch_test_macros.hpp>

#include "llm/api/llm-tool-api.h"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace QaplaLlm;
namespace Json = QaplaTester::Json;

namespace {
    enum class Flavour { Sweet, Sour };

    struct TestRequest {
        std::optional<std::string> name;
        std::optional<uint32_t> count;
        std::optional<double> ratio;
        std::optional<bool> loud;
        bool pick = false;
        std::vector<std::string> items;
        std::optional<Flavour> flavour;
    };

    std::vector<std::pair<std::string, Flavour>> flavourValues() {
        return {{"sweet", Flavour::Sweet}, {"sour", Flavour::Sour}};
    }

    // Tool calls always cross from a worker thread to the UI thread (see GuiToolRegistry), so a
    // test has to play both parts: request from a worker, drain the queue here.
    GuiToolResult callTool(
        GuiToolRegistry& registry, const std::string& name, const std::string& argumentsJson) {
        std::atomic<bool> done{false};
        GuiToolResult result;
        std::thread worker([&]() {
            result = registry.callTool(name, argumentsJson);
            done.store(true, std::memory_order_release);
        });

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!done.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
            registry.processQueue();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        worker.join();

        REQUIRE(done.load());
        return result;
    }
} // namespace

TEST_CASE("defineTool turns a parameter list into a JSON Schema the model can be shown",
    "[llm][tool-api]") {
    GuiToolRegistry registry;
    std::vector<Api::Param<TestRequest>> params;
    params.push_back(Api::stringParam<TestRequest>("name", &TestRequest::name, "A name."));
    params.push_back(Api::integerParam<TestRequest>("count", &TestRequest::count, "How many."));
    params.push_back(Api::numberParam<TestRequest>("ratio", &TestRequest::ratio, "A fraction."));
    params.push_back(Api::boolParam<TestRequest>("loud", &TestRequest::loud, "Shout?"));
    params.push_back(
        Api::stringListParam<TestRequest>("items", &TestRequest::items, "Things.", true));
    params.push_back(
        Api::enumParam<TestRequest>("flavour", &TestRequest::flavour, "Taste.", flavourValues()));

    Api::defineTool<TestRequest>(registry,
        {.name = "sample", .description = "A sample tool.", .params = std::move(params),
            .invoke = [](const TestRequest&) { return Actions::succeeded("done"); }});

    auto specs = registry.exportToolSpecs();
    REQUIRE(specs.size() == 1);
    auto schema = Json::JsonValue::parse(specs.front().parametersSchemaJson);

    REQUIRE(schema.at("type").as_string() == "object");
    const auto& properties = schema.at("properties");
    REQUIRE(properties.at("name").at("type").as_string() == "string");
    REQUIRE(properties.at("count").at("type").as_string() == "integer");
    REQUIRE(properties.at("ratio").at("type").as_string() == "number");
    REQUIRE(properties.at("loud").at("type").as_string() == "boolean");
    REQUIRE(properties.at("items").at("type").as_string() == "array");
    REQUIRE(properties.at("items").at("items").at("type").as_string() == "string");
    REQUIRE(properties.at("name").at("description").as_string() == "A name.");

    // An enum parameter shows its words, not the internal values they map to.
    const auto& flavourEnum = properties.at("flavour").at("enum").as_array();
    REQUIRE(flavourEnum.size() == 2);
    REQUIRE(flavourEnum[0].as_string() == "sweet");
    REQUIRE(flavourEnum[1].as_string() == "sour");

    // Only the parameter marked required appears in "required".
    const auto& required = schema.at("required").as_array();
    REQUIRE(required.size() == 1);
    REQUIRE(required[0].as_string() == "items");
}

TEST_CASE("defineTool reads arguments into the request struct", "[llm][tool-api]") {
    GuiToolRegistry registry;
    TestRequest seen;
    std::vector<Api::Param<TestRequest>> params;
    params.push_back(Api::stringParam<TestRequest>("name", &TestRequest::name, "A name."));
    params.push_back(Api::integerParam<TestRequest>("count", &TestRequest::count, "How many."));
    params.push_back(Api::flagParam<TestRequest>("pick", &TestRequest::pick, "Ask the user."));
    params.push_back(Api::stringListParam<TestRequest>("items", &TestRequest::items, "Things."));
    params.push_back(
        Api::enumParam<TestRequest>("flavour", &TestRequest::flavour, "Taste.", flavourValues()));

    Api::defineTool<TestRequest>(registry,
        {.name = "sample", .description = "A sample tool.", .params = std::move(params),
            .invoke = [&seen](const TestRequest& request) {
                seen = request;
                return Actions::succeeded("done");
            }});

    auto result = callTool(registry, "sample",
        R"({"name":"x","count":7,"pick":true,"items":["a","b"],"flavour":"sour"})");

    REQUIRE(result.success);
    REQUIRE(seen.name.value_or("") == "x");
    REQUIRE(seen.count.value_or(0) == 7);
    REQUIRE(seen.pick);
    REQUIRE(seen.items == std::vector<std::string>{"a", "b"});
    REQUIRE(seen.flavour == Flavour::Sour);
    // Absent parameters stay unset rather than being defaulted, which is what lets a patch-style
    // tool leave every field it wasn't given exactly as it was.
    REQUIRE_FALSE(seen.ratio.has_value());
    REQUIRE_FALSE(seen.loud.has_value());
}

TEST_CASE("defineTool rejects the call when a required parameter is missing or malformed",
    "[llm][tool-api]") {
    GuiToolRegistry registry;
    std::atomic<bool> invoked{false};
    std::vector<Api::Param<TestRequest>> params;
    params.push_back(Api::enumParam<TestRequest>(
        "flavour", &TestRequest::flavour, "Taste.", flavourValues(), true));

    Api::defineTool<TestRequest>(registry,
        {.name = "sample", .description = "A sample tool.", .params = std::move(params),
            .invoke = [&invoked](const TestRequest&) {
                invoked.store(true);
                return Actions::succeeded("done");
            }});

    SECTION("absent") {
        auto result = callTool(registry, "sample", "{}");
        REQUIRE_FALSE(result.success);
        REQUIRE(result.content.find("flavour") != std::string::npos);
        REQUIRE_FALSE(invoked.load());
    }

    SECTION("not one of the allowed words") {
        auto result = callTool(registry, "sample", R"({"flavour":"salty"})");
        REQUIRE_FALSE(result.success);
        REQUIRE(result.content.find("salty") != std::string::npos);
        REQUIRE_FALSE(invoked.load());
    }
}

TEST_CASE("defineTool still runs the action when only an optional parameter is malformed",
    "[llm][tool-api]") {
    // One sloppy field must not cost a whole turn: the good fields are applied, and the problem is
    // reported alongside whatever the action had to say.
    GuiToolRegistry registry;
    TestRequest seen;
    std::vector<Api::Param<TestRequest>> params;
    params.push_back(Api::stringParam<TestRequest>("name", &TestRequest::name, "A name."));
    params.push_back(Api::integerParam<TestRequest>("count", &TestRequest::count, "How many."));

    Api::defineTool<TestRequest>(registry,
        {.name = "sample", .description = "A sample tool.", .params = std::move(params),
            .invoke = [&seen](const TestRequest& request) {
                seen = request;
                return Actions::succeeded("applied what I could");
            }});

    auto result = callTool(registry, "sample", R"({"name":"x","count":"seven"})");

    REQUIRE_FALSE(result.success);
    REQUIRE(seen.name.value_or("") == "x");
    REQUIRE_FALSE(seen.count.has_value());
    REQUIRE(result.content.find("count") != std::string::npos);
    REQUIRE(result.content.find("applied what I could") != std::string::npos);
}

TEST_CASE("defineTool carries an action's widget and turn-ending flag to the wire result",
    "[llm][tool-api]") {
    GuiToolRegistry registry;
    Api::defineTool<TestRequest>(registry,
        {.name = "sample", .description = "A sample tool.",
            .invoke = [](const TestRequest&) {
                return Actions::ActionResult{
                    .ok = false, .text = "nope", .widget = []() {}, .endsTurn = true};
            }});

    auto result = callTool(registry, "sample", "{}");

    REQUIRE_FALSE(result.success);
    REQUIRE(result.content == "nope");
    REQUIRE(result.renderWidget != nullptr);
    REQUIRE(result.terminal);
}

TEST_CASE("a tool with no parameters still advertises an object schema", "[llm][tool-api]") {
    // A bare "{}" is rejected by LM Studio's tool-schema validation ("Invalid discriminator value.
    // Expected 'object'"), so the empty case has to stay explicit -- see noArgsToolSchema().
    GuiToolRegistry registry;
    Api::defineTool<TestRequest>(registry,
        {.name = "sample", .description = "A sample tool.",
            .invoke = [](const TestRequest&) { return Actions::succeeded("done"); }});

    auto schema = Json::JsonValue::parse(registry.exportToolSpecs().front().parametersSchemaJson);
    REQUIRE(schema.at("type").as_string() == "object");
    REQUIRE(schema.at("properties").is_object());
    REQUIRE_FALSE(schema.contains("required"));
}
