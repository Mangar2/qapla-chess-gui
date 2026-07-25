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

#include "lm-studio-client.h"

#include <base-elements/qapla-json.h>

#include <httplib.h>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;

    httplib::Client makeClient(const LmStudioConnection& connection) {
        httplib::Client client(connection.host, connection.port);
        const time_t sec = connection.timeoutMs / 1000;
        const time_t usec = (connection.timeoutMs % 1000) * 1000;
        client.set_connection_timeout(sec, usec);
        client.set_read_timeout(sec, usec);
        client.set_write_timeout(sec, usec);
        return client;
    }

    std::string describeConnectionError(httplib::Error error) {
        return "Could not reach LM Studio server (" + httplib::to_string(error) + ").";
    }

    // LM Studio (like the OpenAI API) reports request-level errors as
    // {"error": {"message": "..."}} or, less commonly, {"error": "..."}.
    std::string extractErrorMessage(const std::string& body) {
        auto parsed = Json::JsonValue::try_parse(body);
        if (!parsed || !parsed->is_object() || !parsed->contains("error")) {
            return "";
        }
        const auto& error = parsed->at("error");
        if (error.is_object() && error.contains("message") && error.at("message").is_string()) {
            return error.at("message").as_string();
        }
        if (error.is_string()) {
            return error.as_string();
        }
        return "";
    }

    std::string describeStatusError(int status, const std::string& body) {
        auto detail = extractErrorMessage(body);
        std::string message = "LM Studio returned status " + std::to_string(status);
        if (!detail.empty()) {
            message += ": " + detail;
        }
        return message;
    }

    Json::JsonValue buildMessageJson(const ChatMessage& message) {
        auto entry = Json::JsonValue::object();
        entry["role"] = message.role;

        if (message.role == "tool") {
            entry["tool_call_id"] = message.toolCallId;
            entry["content"] = message.content;
            return entry;
        }

        entry["content"] = message.content;
        if (message.role == "assistant" && !message.toolCalls.empty()) {
            auto toolCallsJson = Json::JsonValue::array();
            for (const auto& call : message.toolCalls) {
                auto callJson = Json::JsonValue::object();
                callJson["id"] = call.id;
                callJson["type"] = "function";
                auto functionJson = Json::JsonValue::object();
                functionJson["name"] = call.name;
                functionJson["arguments"] = call.argumentsJson;
                callJson["function"] = functionJson;
                toolCallsJson.push_back(callJson);
            }
            entry["tool_calls"] = toolCallsJson;
        }
        return entry;
    }

    Json::JsonValue buildToolsJson(const std::vector<ToolSpec>& tools) {
        auto toolsJson = Json::JsonValue::array();
        for (const auto& tool : tools) {
            auto functionJson = Json::JsonValue::object();
            functionJson["name"] = tool.name;
            functionJson["description"] = tool.description;
            auto parsedParams = Json::JsonValue::try_parse(tool.parametersSchemaJson);
            functionJson["parameters"] = parsedParams ? *parsedParams : Json::JsonValue::object();

            auto entry = Json::JsonValue::object();
            entry["type"] = "function";
            entry["function"] = functionJson;
            toolsJson.push_back(entry);
        }
        return toolsJson;
    }

    // "content" may legitimately be JSON null (or absent) when the model
    // only returns tool_calls; treat anything but a string as empty.
    std::string parseContent(const Json::JsonValue& message) {
        if (!message.contains("content") || !message.at("content").is_string()) {
            return "";
        }
        return message.at("content").as_string();
    }

    std::vector<ToolCall> parseToolCalls(const Json::JsonValue& message) {
        std::vector<ToolCall> calls;
        if (!message.contains("tool_calls") || !message.at("tool_calls").is_array()) {
            return calls;
        }

        for (const auto& callJson : message.at("tool_calls").as_array()) {
            if (!callJson.is_object() || !callJson.contains("function") || !callJson.at("function").is_object()) {
                continue;
            }
            const auto& functionJson = callJson.at("function");

            ToolCall call;
            if (callJson.contains("id") && callJson.at("id").is_string()) {
                call.id = callJson.at("id").as_string();
            }
            if (functionJson.contains("name") && functionJson.at("name").is_string()) {
                call.name = functionJson.at("name").as_string();
            }
            if (functionJson.contains("arguments") && functionJson.at("arguments").is_string()) {
                call.argumentsJson = functionJson.at("arguments").as_string();
            }
            calls.push_back(std::move(call));
        }
        return calls;
    }
}

LmStudioClient::LmStudioClient(LmStudioConnection connection)
    : connection_(std::move(connection)) {
}

ListModelsResult LmStudioClient::listModels() const {
    ListModelsResult result;

    auto client = makeClient(connection_);
    auto res = client.Get("/v1/models");
    if (!res) {
        result.errorMessage = describeConnectionError(res.error());
        return result;
    }
    if (res->status != 200) {
        result.errorMessage = describeStatusError(res->status, res->body);
        return result;
    }

    auto parsed = Json::JsonValue::try_parse(res->body);
    if (!parsed || !parsed->is_object() || !parsed->contains("data") || !parsed->at("data").is_array()) {
        result.errorMessage = "Unexpected response from LM Studio (no model list).";
        return result;
    }

    for (const auto& entry : parsed->at("data").as_array()) {
        if (entry.is_object() && entry.contains("id") && entry.at("id").is_string()) {
            result.modelIds.push_back(entry.at("id").as_string());
        }
    }

    result.success = true;
    return result;
}

ChatCompletionResult LmStudioClient::chatCompletion(const ChatCompletionRequest& request) const {
    ChatCompletionResult result;

    auto body = Json::JsonValue::object();
    body["model"] = request.model;
    body["stream"] = false;

    auto messagesJson = Json::JsonValue::array();
    for (const auto& message : request.messages) {
        messagesJson.push_back(buildMessageJson(message));
    }
    body["messages"] = messagesJson;

    if (!request.tools.empty()) {
        body["tools"] = buildToolsJson(request.tools);
    }

    auto client = makeClient(connection_);
    auto res = client.Post("/v1/chat/completions", body.stringify(), "application/json");
    if (!res) {
        result.errorMessage = describeConnectionError(res.error());
        return result;
    }
    if (res->status != 200) {
        result.errorMessage = describeStatusError(res->status, res->body);
        return result;
    }

    auto parsed = Json::JsonValue::try_parse(res->body);
    if (!parsed || !parsed->is_object() || !parsed->contains("choices") || !parsed->at("choices").is_array()
        || parsed->at("choices").size() == 0) {
        result.errorMessage = "Unexpected response from LM Studio (no choices).";
        return result;
    }

    const auto& firstChoice = parsed->at("choices").as_array()[0];
    if (!firstChoice.is_object() || !firstChoice.contains("message") || !firstChoice.at("message").is_object()) {
        result.errorMessage = "Unexpected response from LM Studio (no message).";
        return result;
    }

    const auto& message = firstChoice.at("message");
    result.content = parseContent(message);
    result.toolCalls = parseToolCalls(message);
    result.success = true;
    return result;
}

} // namespace QaplaLlm
