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

// Deliberately kept free of any GUI/ImGui dependency (no os-dialogs.h,
// no configuration.h): this is the pure, UI-independent logic that the
// unit-tests target links directly. The actions built on it -- including the
// native file dialog and Configuration::setModified() -- are in
// src/llm/actions/gui-action-engines.cpp, which only the qapla executable
// links (its dependencies drag in the whole GUI stack).

#include "gui-tool-engine-management.h"

#include <base-elements/string-helper.h>
#include <engine-handling/engine-config.h>
#include <engine-handling/engine-worker-factory.h>

#include <algorithm>
#include <charconv>
#include <format>
#include <sstream>

namespace QaplaLlm {

namespace {
    namespace Json = QaplaTester::Json;

    using QaplaTester::EngineOption;

    /**
     * @brief Names one of EngineConfig::visitProperties()' keys for a reader.
     *
     * The keys are the ini/CLI spelling, which is deliberately terse; unmapped ones are shown
     * as they are rather than dropped, so a key added to the settings definitions still turns up
     * here instead of quietly disappearing from every report.
     */
    std::string readableLabel(const std::string& key) {
        if (key == "cmd") { return "Executable"; }
        if (key == "dir") { return "Working directory"; }
        if (key == "proto") { return "Protocol"; }
        if (key == "tc") { return "Time control"; }
        if (key == "args") { return "Arguments"; }
        if (key == "whitepov") { return "Score from white's POV"; }
        if (key == "originalName") { return "Reported name"; }
        if (key == "gauntlet") { return "Gauntlet"; }
        if (key == "restart") { return "Restart"; }
        if (key == "trace") { return "Trace"; }
        if (key == "ponder") { return "Ponder"; }
        return key;
    }

    /**
     * @brief Trims a message written for the command line down to what applies here.
     *
     * The property accessors are shared with qapla-engine-tester's CLI and phrase their
     * complaints for it: several lines, ending in "Use --help to display all supported
     * parameters". The list of valid values in the middle is exactly what a caller here needs;
     * the invitation to run --help is advice about a program nobody is running.
     */
    std::string forThisCaller(std::string message) {
        if (const auto hint = message.find("Hint: Use --help"); hint != std::string::npos) {
            message.erase(hint);
        }
        std::string flattened;
        flattened.reserve(message.size());
        for (const char character : message) {
            const bool isBreak = character == '\n' || character == '\r';
            if (isBreak || character == ' ') {
                if (!flattened.empty() && flattened.back() != ' ') {
                    flattened += ' ';
                }
                continue;
            }
            flattened += character;
        }
        while (!flattened.empty() && flattened.back() == ' ') {
            flattened.pop_back();
        }
        return flattened;
    }

    /** @brief "spin, default 16, 1..65536" -- everything a caller needs to pick a legal value. */
    std::string describeDomain(const EngineOption& option) {
        std::string text = EngineOption::to_string(option.type);
        if (!option.defaultValue.empty()) {
            text += std::format(", default {}", option.defaultValue);
        }
        if (option.min && option.max) {
            text += std::format(", {}..{}", *option.min, *option.max);
        } else if (option.min) {
            text += std::format(", at least {}", *option.min);
        } else if (option.max) {
            text += std::format(", at most {}", *option.max);
        }
        if (!option.vars.empty()) {
            text += ", one of: ";
            for (std::size_t i = 0; i < option.vars.size(); ++i) {
                text += (i == 0 ? "" : ", ") + option.vars[i];
            }
        }
        return text;
    }

    /**
     * @brief Checks one value against one option's declared domain.
     * @return The reason it does not fit, or "" if it does.
     */
    std::string domainViolation(const EngineOption& option, const std::string& value) {
        switch (option.type) {
            case EngineOption::Type::Button:
            case EngineOption::Type::Save:
            case EngineOption::Type::Reset:
                // A command, not a setting: it is pressed, and there is nothing to store.
                return "it is a " + EngineOption::to_string(option.type) +
                    ", which takes no value";

            case EngineOption::Type::Check: {
                const auto lower = QaplaHelpers::to_lowercase(value);
                return (lower == "true" || lower == "false") ? "" : "it takes true or false";
            }

            case EngineOption::Type::Spin:
            case EngineOption::Type::Slider: {
                int number = 0;
                const auto* first = value.data();
                const auto* last = first + value.size();
                const auto parsed = std::from_chars(first, last, number);
                if (parsed.ec != std::errc{} || parsed.ptr != last) {
                    return "it takes a whole number";
                }
                if (option.min && number < *option.min) {
                    return std::format("its lowest value is {}", *option.min);
                }
                if (option.max && number > *option.max) {
                    return std::format("its highest value is {}", *option.max);
                }
                return "";
            }

            case EngineOption::Type::Combo: {
                if (option.vars.empty()) {
                    return "";
                }
                const auto lower = QaplaHelpers::to_lowercase(value);
                const bool known = std::ranges::any_of(option.vars, [&](const auto& var) {
                    return QaplaHelpers::to_lowercase(var) == lower;
                });
                return known ? "" : "it is a choice between " + describeDomain(option);
            }

            default:
                // File, Path, String and anything a future protocol adds: the engine is the only
                // judge of what is valid, so passing it through beats guessing a rule here.
                return "";
        }
    }
} // namespace

std::string listInstalledEnginesJson() {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();

    auto array = Json::JsonValue::array();
    for (const auto& config : configManager.getAllConfigs()) {
        auto entry = Json::JsonValue::object();
        entry["name"] = config.getName();
        entry["protocol"] = QaplaTester::to_string(config.getProtocol());
        array.push_back(entry);
    }
    return array.stringify();
}

AddEnginesOutcome addEnginesFromPaths(const std::vector<std::string>& paths) {
    AddEnginesOutcome outcome;

    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    auto existing = configManager.getAllConfigs();

    for (const auto& path : paths) {
        auto newConfig = QaplaTester::EngineConfig::createFromPath(path);

        bool isDuplicate = std::ranges::any_of(existing, [&](const auto& config) {
            return config.getCmd() == newConfig.getCmd() && config.getProtocol() == newConfig.getProtocol();
        });
        if (isDuplicate) {
            outcome.duplicateNames.push_back(newConfig.getName());
            continue;
        }

        configManager.addConfig(newConfig);
        existing.push_back(newConfig); // keep the dedup view in sync for the rest of this call
        outcome.addedNames.push_back(newConfig.getName());
    }

    return outcome;
}

AddNamedEnginesOutcome addNamedEngines(const std::vector<NamedEnginePath>& engines) {
    AddNamedEnginesOutcome outcome;

    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    for (const auto& engine : engines) {
        if (engine.name.empty() || engine.path.empty()) {
            continue;
        }
        if (configManager.getConfig(engine.name) != nullptr) {
            outcome.takenNames.push_back(engine.name);
            continue;
        }

        auto config = QaplaTester::EngineConfig::createFromPath(engine.path);
        config.setName(engine.name);
        configManager.addConfig(config);
        outcome.addedNames.push_back(engine.name);
    }

    return outcome;
}

std::vector<std::string> unsetEngineOptions(
    QaplaTester::EngineConfig& config, const std::vector<std::string>& names) {
    // Read the spellings first: removeOptionValue() matches case-insensitively, but what the
    // caller should be told back is what the engine calls the option, not what was typed.
    const auto values = config.getOptionValues();
    std::vector<std::string> removed;

    for (const auto& name : names) {
        const auto wanted = QaplaHelpers::to_lowercase(name);
        const auto match = std::ranges::find_if(values, [&](const auto& entry) {
            return QaplaHelpers::to_lowercase(entry.first) == wanted;
        });
        if (match != values.end() && config.removeOptionValue(name)) {
            removed.push_back(match->first);
        }
    }

    return removed;
}

const std::vector<std::string>& settableEngineKeys() {
    // "name" is deliberately absent: the catalog is keyed by it, so a rename needs the same
    // collision check a copy does and belongs with copyCatalogEngine() rather than among the
    // properties. "author" and "originalName" are absent because detection owns them, and
    // "selected" because it is deprecated.
    static const std::vector<std::string> keys{
        "cmd", "dir", "args", "proto", "tc", "trace", "restart", "ponder", "gauntlet", "whitepov"};
    return keys;
}

ApplyOptionsOutcome applyEngineProperties(
    QaplaTester::EngineConfig& config, const std::vector<EngineAssignment>& assignments) {
    ApplyOptionsOutcome outcome;

    for (const auto& assignment : assignments) {
        const auto wanted = QaplaHelpers::to_lowercase(assignment.name);
        const auto& keys = settableEngineKeys();
        if (std::ranges::find(keys, wanted) == keys.end()) {
            outcome.unknown.push_back(assignment.name);
            continue;
        }
        try {
            config.setValue(wanted, assignment.value);
            outcome.applied.push_back(std::format("{} = {}", wanted, assignment.value));
        } catch (const std::exception& ex) {
            // setProtocol() and setTimeControl() throw on what they cannot parse. Caught per key
            // so one unusable value costs only itself, and reported with the accessor's own words,
            // which name the alternatives ("uci", "xboard") better than anything phrased here.
            outcome.rejected.push_back(
                std::format("{} = {} ({})", wanted, assignment.value, forThisCaller(ex.what())));
        }
    }

    return outcome;
}

CopyEngineOutcome copyCatalogEngine(const std::string& sourceName, const std::string& newName) {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();

    const auto* source = configManager.getConfig(sourceName);
    if (source == nullptr) {
        return {.sourceMissing = true, .nameTaken = false};
    }
    if (configManager.getConfig(newName) != nullptr) {
        return {.sourceMissing = false, .nameTaken = true};
    }

    auto copy = *source;
    copy.setName(newName);
    configManager.addConfig(copy);
    return {};
}

bool deleteCatalogEngine(const std::string& name) {
    auto& configManager = QaplaTester::EngineWorkerFactory::getConfigManagerMutable();
    const auto* config = configManager.getConfig(name);
    if (config == nullptr) {
        return false;
    }
    configManager.removeConfig(*config);
    return true;
}

std::string engineDetailsText(const QaplaTester::EngineConfig& config,
    const QaplaConfiguration::EngineCapabilities& capabilities) {
    std::ostringstream out;
    out << std::format("Engine \"{}\":\n", config.getName());

    config.visitProperties([&out](const std::string& key, const std::string& value) {
        if (value.empty()) {
            return;
        }
        out << std::format("  {}: {}\n", readableLabel(key), value);
    });

    const auto values = config.getOptionValues();
    if (values.empty()) {
        out << "  Set options: none -- every option is at the engine's own default.\n";
    } else {
        out << "  Set options:\n";
        for (const auto& [name, value] : values) {
            out << std::format("    {} = {}\n", name, value);
        }
    }

    const auto capability = capabilities.getCapability(config.getCmd(), config.getProtocol());
    if (!capability) {
        // Deliberately says what is missing without saying what to do about it: installing an
        // engine detects it, so a caller reaching this has an engine whose program did not answer,
        // and "detect it" would send it round a loop that cannot close.
        out << "  Supported options: unknown -- this program has not reported them.\n";
        return out.str();
    }

    const auto& supported = capability->getSupportedOptions();
    if (supported.empty()) {
        out << "  Supported options: none -- this program offers no options to set.\n";
        return out.str();
    }

    out << "  Supported options:\n";
    for (const auto& option : supported) {
        out << std::format("    {} ({})\n", option.name, describeDomain(option));
    }
    return out.str();
}

ApplyOptionsOutcome applyEngineOptions(QaplaTester::EngineConfig& config,
    const std::vector<EngineAssignment>& assignments,
    const QaplaConfiguration::EngineCapabilities& capabilities) {
    ApplyOptionsOutcome outcome;

    const auto capability = capabilities.getCapability(config.getCmd(), config.getProtocol());
    if (!capability) {
        outcome.detected = false;
        return outcome;
    }

    const auto& supported = capability->getSupportedOptions();
    for (const auto& assignment : assignments) {
        const auto wanted = QaplaHelpers::to_lowercase(assignment.name);
        const auto match = std::ranges::find_if(supported, [&](const EngineOption& option) {
            return QaplaHelpers::to_lowercase(option.name) == wanted;
        });
        if (match == supported.end()) {
            outcome.unknown.push_back(assignment.name);
            continue;
        }
        if (const auto violation = domainViolation(*match, assignment.value); !violation.empty()) {
            outcome.rejected.push_back(
                std::format("{} = {} ({})", match->name, assignment.value, violation));
            continue;
        }
        config.setOptionValue(match->name, assignment.value);
        outcome.applied.push_back(std::format("{} = {}", match->name, assignment.value));
    }

    return outcome;
}

} // namespace QaplaLlm
