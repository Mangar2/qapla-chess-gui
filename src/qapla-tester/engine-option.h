/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cassert>
#include "app-error.h"

namespace QaplaTester {

enum class RestartOption {
	EngineDecides,
	Always,
	Never
};

inline std::string to_string(RestartOption restart) {
	switch (restart) {
	case RestartOption::Always: return "on";
	case RestartOption::Never: return "off";
	default: return "auto";
	}
}

inline RestartOption parseRestartOption(const std::string& value) {
	if (value == "auto") return RestartOption::EngineDecides;
	if (value == "on") return RestartOption::Always;
	if (value == "off") return RestartOption::Never;
	AppError::throwOnInvalidOption({ "auto", "on", "off" }, value, "restart option");	
	return RestartOption::EngineDecides;
}

enum class EngineProtocol {
	Uci,
	XBoard,
	Unknown
};

inline std::string to_string(EngineProtocol protocol) {
	switch (protocol) {
	case EngineProtocol::Uci: return "uci";
	case EngineProtocol::XBoard: return "xboard";
	default: return "unknown";
	}
}

inline EngineProtocol parseEngineProtocol(const std::string& value) {
	if (value == "uci") return EngineProtocol::Uci;
	if (value == "xboard") return EngineProtocol::XBoard;
	AppError::throwOnInvalidOption({ "uci", "xboard" }, value, "protocol option");
	return EngineProtocol::Uci;
}

 /**
  * Represents an option that can be set for a chess engine.
  */
struct EngineOption {
    enum class Type { File, Path, Check, Spin, Slider, Combo, Button, Save, Reset, String, Unknown };

    std::string name;
    Type type = Type::Unknown;
    std::string defaultValue;
    std::optional<int> min;
    std::optional<int> max;
    std::vector<std::string> vars;

	static EngineOption::Type parseType(const std::string& typeStr) {
		if (typeStr == "button") return EngineOption::Type::Button;
		if (typeStr == "save") return EngineOption::Type::Save;
		if (typeStr == "reset") return EngineOption::Type::Reset;
		if (typeStr == "check")  return EngineOption::Type::Check;
		if (typeStr == "string") return EngineOption::Type::String;
		if (typeStr == "file")   return EngineOption::Type::File;
		if (typeStr == "path")   return EngineOption::Type::Path;
		if (typeStr == "spin")   return EngineOption::Type::Spin;
		if (typeStr == "slider") return EngineOption::Type::Spin;
		if (typeStr == "combo")  return EngineOption::Type::Combo;
		return EngineOption::Type::Unknown;
	}

	static std::string to_string(Type type) {
		switch (type) {
		case Type::File: return "file";
		case Type::Path: return "path";
		case Type::Check: return "check";
		case Type::Spin: return "spin";
		case Type::Slider: return "slider";
		case Type::Combo: return "combo";
		case Type::Button: return "button";
		case Type::Save: return "save";
		case Type::Reset: return "reset";
		case Type::String: return "string";
		default: return "unknown";
		}
	}

};

using EngineOptions = std::vector<EngineOption>;

} // namespace QaplaTester
