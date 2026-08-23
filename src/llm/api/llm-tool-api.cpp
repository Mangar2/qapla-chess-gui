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

#include "llm-tool-api.h"

#include <utility>

namespace QaplaLlm::Api::Detail {

namespace {
    namespace Json = QaplaTester::Json;

    std::string schemaTypeName(ParamType type) {
        switch (type) {
            case ParamType::Integer: return "integer";
            case ParamType::Number: return "number";
            case ParamType::Boolean: return "boolean";
            case ParamType::StringArray: return "array";
            case ParamType::StringMap: return "object";
            case ParamType::String:
            default: return "string";
        }
    }

    // Deliberately phrased for a reader, not a parser: the model is the only consumer of these
    // messages, and it recovers far better from "must be a whole number" than from a type name
    // out of a schema it has already been shown once.
    std::string expectationText(ParamType type) {
        switch (type) {
            case ParamType::Integer: return "a whole number";
            case ParamType::Number: return "a number";
            case ParamType::Boolean: return "true or false";
            case ParamType::StringArray: return "a non-empty list of text values";
            case ParamType::StringMap: return "a non-empty object of name/value pairs";
            case ParamType::String:
            default: return "text";
        }
    }
} // namespace

std::optional<std::string> asText(const Json::JsonValue& value) {
    if (value.is_string()) {
        return value.as_string();
    }
    if (value.is_boolean()) {
        return value.as_boolean() ? "true" : "false";
    }
    if (value.is_number()) {
        // Whole numbers go back as whole numbers: an engine shown "Hash 128.000000" would be
        // within its rights to reject it, and every spin option is an integer to begin with.
        const double number = value.as_number();
        if (number == static_cast<double>(static_cast<long long>(number))) {
            return std::to_string(static_cast<long long>(number));
        }
        return std::to_string(number);
    }
    return std::nullopt;
}

Json::JsonValue makeProperty(
    ParamType type, const std::string& description, const std::vector<std::string>& allowedValues) {
    auto property = Json::JsonValue::object();
    property["type"] = schemaTypeName(type);
    if (type == ParamType::StringArray) {
        auto items = Json::JsonValue::object();
        items["type"] = "string";
        property["items"] = items;
    }
    if (type == ParamType::StringMap) {
        // Says "any key, text value" in the one way a JSON Schema can. The keys themselves cannot
        // be listed here -- they belong to whichever engine is being configured -- so the tool
        // description points at manage_engines' "details" command for them instead.
        auto values = Json::JsonValue::object();
        values["type"] = "string";
        property["additionalProperties"] = values;
    }
    if (!allowedValues.empty()) {
        auto values = Json::JsonValue::array();
        for (const auto& value : allowedValues) {
            values.push_back(value);
        }
        property["enum"] = values;
    }
    property["description"] = description;
    return property;
}

std::string wrongTypeProblem(const std::string& name, ParamType type) {
    return "\"" + name + "\" must be " + expectationText(type);
}

std::string missingProblem(const std::string& name, const std::vector<std::string>& allowedValues) {
    std::string problem = "\"" + name + "\" is required";
    if (!allowedValues.empty()) {
        problem += " (one of: " + Actions::joinList(allowedValues) + ")";
    }
    return problem;
}

std::string badEnumProblem(
    const std::string& name, const std::vector<std::string>& allowedValues, const std::string& given) {
    return "\"" + name + "\" must be one of: " + Actions::joinList(allowedValues) + " (got \"" +
        given + "\")";
}

std::string problemsSentence(const std::vector<std::string>& problems) {
    if (problems.empty()) {
        return "";
    }
    return "Problems with the arguments: " + Actions::joinList(problems) + ".";
}

std::string remedyInstruction(Actions::Remedy remedy) {
    // The parameter and tool names below are declared in src/llm/tools/ (gui-tools-shared.h's
    // openingsFileDialogParam(), gui-tools-epd.cpp's epd_file_dialog, gui-tools-activity.cpp's
    // clear_result). Renaming one of those means updating it here too -- which is the whole point
    // of routing this through Actions::Remedy: the wording lives with the rest of the external
    // API instead of being scattered through the GUI code, where it would silently rot.
    switch (remedy) {
        case Actions::Remedy::AskUserForOpeningsFile:
            return " Set openings_file_dialog=true to let the user pick a valid one.";
        case Actions::Remedy::AskUserForEpdFile:
            return " Set epd_file_dialog=true to let the user pick a valid one.";
        case Actions::Remedy::ClearEpdResultFirst:
            return " Call clear_result with type=\"epd\", then call start again with type=\"epd\".";
        case Actions::Remedy::None:
        default:
            return "";
    }
}

GuiToolResult toGuiToolResult(Actions::ActionResult result) {
    GuiToolResult converted{.success = result.ok,
        .content = std::move(result.text) + remedyInstruction(result.remedy),
        .renderWidget = std::move(result.widget), .terminal = result.endsTurn};

    // An action that handed its waiting back (see ActionResult::awaitOffUiThread) carries its
    // continuation over as well, converted the same way when it eventually runs.
    if (result.awaitOffUiThread) {
        converted.awaitOffUiThread = std::move(result.awaitOffUiThread);
        converted.continuation = [step = std::move(result.continuation)]() -> GuiToolResult {
            return step ? toGuiToolResult(step()) : GuiToolResult{};
        };
    }
    return converted;
}

} // namespace QaplaLlm::Api::Detail
