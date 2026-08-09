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

#include "../actions/gui-action-types.h"
#include "../gui-tool-registry.h"

#include <base-elements/qapla-json.h>

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/**
 * @file
 * @brief The MAPPER between the model-facing (external) tool API and the internal action API.
 *
 * This is the only place in src/llm/ that knows about both JSON and GUI actions, and it knows
 * nothing about any *particular* tool. A tool is declared once, as data, in src/llm/tools/:
 *
 * @code
 * struct ConfigureRequest { std::optional<uint32_t> games; };
 *
 * defineTool<ConfigureRequest>(registry, {
 *     .name = "configure_tournament",
 *     .description = "...what the model is told...",
 *     .params = { integerParam("games", &ConfigureRequest::games, "Games per pairing.") },
 *     .invoke = [](const ConfigureRequest& request) { return Actions::configureTournament(...); }
 * });
 * @endcode
 *
 * One `params` line is the whole external contract for a field: it produces the JSON Schema
 * property the model is shown *and* the code that reads that field back out of the model's
 * arguments. There is no second place to keep in sync, which is the point -- reshaping the
 * external API (renaming a field, changing an enum's wording, splitting or merging tools) never
 * reaches past `.invoke` into the actions layer.
 *
 * Argument handling is deliberately asymmetric, because the two failure modes are not alike:
 * - a *required* parameter that is missing or of the wrong type aborts the call, since running
 *   the action anyway would act on something the user never asked for;
 * - an *optional* parameter of the wrong type is reported but does not abort, so a model that
 *   sends `games: "ten"` alongside four good fields still gets the four good fields applied --
 *   matching how these tools behaved before, and keeping one sloppy field from wasting a turn.
 * Semantic validation ("games must be at least 1") is NOT done here: it belongs to the action,
 * which is what actually knows the rule.
 */

namespace QaplaLlm::Api {

/** @brief JSON Schema type of a parameter, as the model is shown it. */
enum class ParamType { String, Integer, Number, Boolean, StringArray };

/**
 * @brief One model-facing tool parameter: how it is described, and how it is read.
 *
 * Build these with the factories below rather than by hand; use customParam() for the rare
 * parameter whose external shape does not map 1:1 onto one field of the request struct.
 */
template <class Request>
struct Param {
    std::string name;
    std::string description;
    ParamType type = ParamType::String;

    /** @brief JSON Schema "enum" values; empty means unconstrained. */
    std::vector<std::string> allowedValues{};

    /** @brief Whether the call is rejected outright when this parameter is absent or malformed. */
    bool required = false;

    /**
     * @brief Reads this parameter's value into `request`, appending to `problems` if it can't.
     *
     * Only ever called with a value that is present and non-null, so implementations only need to
     * check the value's type.
     */
    std::function<void(Request&, const QaplaTester::Json::JsonValue& value,
        std::vector<std::string>& problems)>
        read;
};

/**
 * @brief One model-facing tool, declared as data.
 *
 * `invoke` is the seam: it is the only member allowed to know the internal action API, and it is
 * where an external-only decision (dispatching a unified "start" tool by its `type` field,
 * appending a cross-feature status line to every response) belongs.
 */
template <class Request>
struct ToolDef {
    std::string name;
    std::string description;
    std::vector<Param<Request>> params{};
    std::function<Actions::ActionResult(const Request&)> invoke;

    /** @brief See GuiToolDefinition::timeout -- raise it for anything that waits on a human. */
    std::chrono::milliseconds timeout = std::chrono::seconds(30);
};

namespace Detail {

/** @brief Builds one JSON Schema property object. */
[[nodiscard]] QaplaTester::Json::JsonValue makeProperty(
    ParamType type, const std::string& description, const std::vector<std::string>& allowedValues);

/** @brief "\"games\" must be a whole number" -- the message for a value of the wrong JSON type. */
[[nodiscard]] std::string wrongTypeProblem(const std::string& name, ParamType type);

/** @brief "\"type\" is required (one of: tournament, sprt, epd)" -- for an absent required value. */
[[nodiscard]] std::string missingProblem(
    const std::string& name, const std::vector<std::string>& allowedValues);

/** @brief "\"mode\" must be one of: graceful, abrupt (got \"soft\")" -- for an unknown enum value. */
[[nodiscard]] std::string badEnumProblem(
    const std::string& name, const std::vector<std::string>& allowedValues, const std::string& given);

/** @brief Renders collected argument problems as one sentence to send back to the model. */
[[nodiscard]] std::string problemsSentence(const std::vector<std::string>& problems);

/**
 * @brief Says what an Actions::Remedy means in terms of the *current* external API.
 *
 * The one place a failure message is allowed to name a tool or a parameter -- see Actions::Remedy
 * for why the actions themselves must not. Returns "" for Remedy::None; otherwise a sentence that
 * starts with a space, ready to append to the failure text.
 */
[[nodiscard]] std::string remedyInstruction(Actions::Remedy remedy);

/** @brief Carries an internal ActionResult over to the wire-level GuiToolResult. */
[[nodiscard]] GuiToolResult toGuiToolResult(Actions::ActionResult result);

} // namespace Detail

/**
 * @brief Registers a declared tool: builds its JSON Schema, its argument reader, and its handler.
 */
template <class Request>
void defineTool(GuiToolRegistry& registry, ToolDef<Request> tool) {
    auto schema = noArgsToolSchema();
    auto requiredNames = QaplaTester::Json::JsonValue::array();
    for (const auto& param : tool.params) {
        schema["properties"][param.name] =
            Detail::makeProperty(param.type, param.description, param.allowedValues);
        if (param.required) {
            requiredNames.push_back(param.name);
        }
    }
    if (requiredNames.size() > 0) {
        schema["required"] = requiredNames;
    }

    registry.registerTool(GuiToolDefinition{
        .name = std::move(tool.name),
        .description = std::move(tool.description),
        .parametersSchema = std::move(schema),
        .handler = [params = std::move(tool.params), invoke = std::move(tool.invoke)](
                       const QaplaTester::Json::JsonValue& arguments) -> GuiToolResult {
            Request request{};
            std::vector<std::string> problems;
            bool abort = false;

            for (const auto& param : params) {
                if (!arguments.contains(param.name) || arguments.at(param.name).is_null()) {
                    if (param.required) {
                        problems.push_back(Detail::missingProblem(param.name, param.allowedValues));
                        abort = true;
                    }
                    continue;
                }
                auto problemsBefore = problems.size();
                param.read(request, arguments.at(param.name), problems);
                if (param.required && problems.size() != problemsBefore) {
                    abort = true;
                }
            }

            if (abort) {
                return GuiToolResult{.success = false, .content = Detail::problemsSentence(problems)};
            }

            auto result = Detail::toGuiToolResult(invoke(request));
            if (!problems.empty()) {
                result.content = Detail::problemsSentence(problems) + " " + result.content;
                result.success = false;
            }
            return result;
        },
        .timeout = tool.timeout});
}

// ---------------------------------------------------------------------------
// Parameter factories -- one call declares a field's schema and its reader.
// ---------------------------------------------------------------------------

/** @brief Text parameter bound to an optional field (absent = leave the field untouched). */
template <class Request>
[[nodiscard]] Param<Request> stringParam(std::string name,
    std::optional<std::string> Request::* field, std::string description, bool required = false) {
    Param<Request> param{.name = std::move(name), .description = std::move(description),
        .type = ParamType::String, .required = required};
    param.read = [field, name = param.name](Request& request,
                     const QaplaTester::Json::JsonValue& value, std::vector<std::string>& problems) {
        if (!value.is_string()) {
            problems.push_back(Detail::wrongTypeProblem(name, ParamType::String));
            return;
        }
        request.*field = value.as_string();
    };
    return param;
}

/** @brief Text parameter bound to a plain field -- for a required value with a sane default. */
template <class Request>
[[nodiscard]] Param<Request> stringParam(std::string name, std::string Request::* field,
    std::string description, bool required = false) {
    Param<Request> param{.name = std::move(name), .description = std::move(description),
        .type = ParamType::String, .required = required};
    param.read = [field, name = param.name](Request& request,
                     const QaplaTester::Json::JsonValue& value, std::vector<std::string>& problems) {
        if (!value.is_string()) {
            problems.push_back(Detail::wrongTypeProblem(name, ParamType::String));
            return;
        }
        request.*field = value.as_string();
    };
    return param;
}

/**
 * @brief Numeric parameter bound to an optional field of any arithmetic type.
 *
 * `type` only decides what the model is shown (integer vs number); the value is always read as a
 * JSON number and converted to the field's own type, so range/step rules stay with the action.
 */
template <class Request, class Number>
[[nodiscard]] Param<Request> numericParam(std::string name, std::optional<Number> Request::* field,
    std::string description, ParamType type) {
    Param<Request> param{
        .name = std::move(name), .description = std::move(description), .type = type};
    param.read = [field, name = param.name, type](Request& request,
                     const QaplaTester::Json::JsonValue& value, std::vector<std::string>& problems) {
        if (!value.is_number()) {
            problems.push_back(Detail::wrongTypeProblem(name, type));
            return;
        }
        request.*field = static_cast<Number>(value.as_number());
    };
    return param;
}

/** @brief Whole-number parameter (JSON Schema "integer"). */
template <class Request, class Number>
[[nodiscard]] Param<Request> integerParam(
    std::string name, std::optional<Number> Request::* field, std::string description) {
    return numericParam(std::move(name), field, std::move(description), ParamType::Integer);
}

/** @brief Fractional-number parameter (JSON Schema "number"). */
template <class Request, class Number>
[[nodiscard]] Param<Request> numberParam(
    std::string name, std::optional<Number> Request::* field, std::string description) {
    return numericParam(std::move(name), field, std::move(description), ParamType::Number);
}

/** @brief Boolean parameter bound to an optional field (absent = leave the field untouched). */
template <class Request>
[[nodiscard]] Param<Request> boolParam(
    std::string name, std::optional<bool> Request::* field, std::string description) {
    Param<Request> param{.name = std::move(name), .description = std::move(description),
        .type = ParamType::Boolean};
    param.read = [field, name = param.name](Request& request,
                     const QaplaTester::Json::JsonValue& value, std::vector<std::string>& problems) {
        if (!value.is_boolean()) {
            problems.push_back(Detail::wrongTypeProblem(name, ParamType::Boolean));
            return;
        }
        request.*field = value.as_boolean();
    };
    return param;
}

/** @brief Boolean parameter bound to a plain bool -- for an opt-in flag defaulting to false. */
template <class Request>
[[nodiscard]] Param<Request> flagParam(
    std::string name, bool Request::* field, std::string description) {
    Param<Request> param{.name = std::move(name), .description = std::move(description),
        .type = ParamType::Boolean};
    param.read = [field, name = param.name](Request& request,
                     const QaplaTester::Json::JsonValue& value, std::vector<std::string>& problems) {
        if (!value.is_boolean()) {
            problems.push_back(Detail::wrongTypeProblem(name, ParamType::Boolean));
            return;
        }
        request.*field = value.as_boolean();
    };
    return param;
}

/**
 * @brief Array-of-strings parameter.
 *
 * Non-string entries are dropped with a problem noted rather than failing the whole list, so a
 * model that mixes one stray value into an otherwise good list still gets the good entries.
 */
template <class Request>
[[nodiscard]] Param<Request> stringListParam(std::string name,
    std::vector<std::string> Request::* field, std::string description, bool required = false) {
    Param<Request> param{.name = std::move(name), .description = std::move(description),
        .type = ParamType::StringArray, .required = required};
    param.read = [field, name = param.name](Request& request,
                     const QaplaTester::Json::JsonValue& value, std::vector<std::string>& problems) {
        if (!value.is_array()) {
            problems.push_back(Detail::wrongTypeProblem(name, ParamType::StringArray));
            return;
        }
        std::vector<std::string> entries;
        for (const auto& item : value.as_array()) {
            if (item.is_string()) {
                entries.push_back(item.as_string());
            }
        }
        if (entries.empty()) {
            problems.push_back(Detail::wrongTypeProblem(name, ParamType::StringArray));
            return;
        }
        request.*field = std::move(entries);
    };
    return param;
}

/**
 * @brief Enum parameter: a fixed set of model-facing words, each mapped to an internal value.
 *
 * The words and the internal values are declared side by side precisely so the words can be
 * changed (or a synonym added) without the action ever hearing about it.
 */
template <class Request, class Enum>
[[nodiscard]] Param<Request> enumParam(std::string name, std::optional<Enum> Request::* field,
    std::string description, std::vector<std::pair<std::string, Enum>> values,
    bool required = false) {
    Param<Request> param{.name = std::move(name), .description = std::move(description),
        .type = ParamType::String, .required = required};
    for (const auto& [word, _] : values) {
        param.allowedValues.push_back(word);
    }
    param.read = [field, values = std::move(values), name = param.name,
                     allowed = param.allowedValues](Request& request,
                     const QaplaTester::Json::JsonValue& value, std::vector<std::string>& problems) {
        if (!value.is_string()) {
            problems.push_back(Detail::wrongTypeProblem(name, ParamType::String));
            return;
        }
        for (const auto& [word, mapped] : values) {
            if (word == value.as_string()) {
                request.*field = mapped;
                return;
            }
        }
        problems.push_back(Detail::badEnumProblem(name, allowed, value.as_string()));
    };
    return param;
}

/**
 * @brief Escape hatch for a parameter whose external shape does not map 1:1 onto one field.
 *
 * Use it when the mapping itself is the external decision -- e.g. one model-facing field that
 * has to be split across two internal ones -- and keep the arithmetic here rather than teaching
 * the action about the external shape.
 */
template <class Request>
[[nodiscard]] Param<Request> customParam(std::string name, ParamType type, std::string description,
    std::function<void(Request&, const QaplaTester::Json::JsonValue&, std::vector<std::string>&)> read,
    std::vector<std::string> allowedValues = {}, bool required = false) {
    return Param<Request>{.name = std::move(name), .description = std::move(description),
        .type = type, .allowedValues = std::move(allowedValues), .required = required,
        .read = std::move(read)};
}

} // namespace QaplaLlm::Api
