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

#include "tournament-config-sections.h"

#include <base-elements/string-helper.h>
#include <engine-handling/engine-option.h>

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace QaplaConfiguration {

namespace {

    /**
     * @brief Looks a key up case-insensitively.
     *
     * Section keys reach the GUI in whatever case they were written in: the CLI lowercases
     * every key it parses, while "option.Hash" keeps the casing of the UCI option it names.
     * @param section The section to search.
     * @param lowercaseKeys The keys to look for, already lowercased, in order of preference.
     * @return The first value found, or std::nullopt.
     */
    [[nodiscard]] std::optional<std::string> findValue(
        const QaplaHelpers::IniFile::Section& section,
        std::initializer_list<std::string_view> lowercaseKeys) {
        for (const auto& wanted : lowercaseKeys) {
            for (const auto& [key, value] : section.entries) {
                if (QaplaHelpers::to_lowercase(key) == wanted) {
                    return value;
                }
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool toBool(const std::string& value) {
        return value == "true" || value == "1";
    }

    /**
     * @brief Decides whether a global setting is switched on.
     * @param section The section to read.
     * @param useKey The explicit flag key of the GUI's own configuration file.
     * @param hasValue Whether the section carries the setting's value.
     * @return The explicit flag if present, otherwise whether the value is present.
     */
    [[nodiscard]] bool isUsed(const QaplaHelpers::IniFile::Section& section,
        std::string_view useKey, bool hasValue) {
        const auto flag = findValue(section, { useKey });
        return flag ? toBool(*flag) : hasValue;
    }

    /**
     * @brief Names the GUI's on/off switch for a global setting.
     * Keep in step with fromEachSection(), which reads the same four switches.
     * @param lowercaseKey The "each" key, lowercased.
     * @return The flag key, or empty for a setting the GUI cannot switch off.
     */
    [[nodiscard]] std::string_view useFlagFor(std::string_view lowercaseKey) {
        if (lowercaseKey == "option.hash" || lowercaseKey == "hash") { return "usehash"; }
        if (lowercaseKey == "ponder") { return "useponder"; }
        if (lowercaseKey == "trace") { return "usetrace"; }
        if (lowercaseKey == "restart") { return "userestart"; }
        return {};
    }

    /**
     * @brief Tells whether the GUI has a global control for a setting.
     * The CLI's "each" takes any engine option; the GUI's global panel is the four settings with a
     * switch plus the time control. Anything else has to stay with the engines, because
     * toEachSection() would not write it back and it would be lost on the next save.
     * @param lowercaseKey The "each" key, lowercased.
     */
    [[nodiscard]] bool isGlobalSetting(std::string_view lowercaseKey) {
        return !useFlagFor(lowercaseKey).empty() || lowercaseKey == "tc";
    }

    void eraseKey(QaplaHelpers::IniFile::Section& section, std::string_view lowercaseKey) {
        std::erase_if(section.entries, [&](const auto& entry) {
            return QaplaHelpers::to_lowercase(entry.first) == lowercaseKey;
        });
    }

    /**
     * @brief Spells a restart mode the way the file format defines it.
     *
     * The GUI's control offers "Engine decides", "Always" and "Never"; the file knows "auto",
     * "on" and "off". Both are understood on reading, but only one belongs in a file.
     * @param value The value as the control left it.
     * @return The file spelling, or the value lowercased if it names no known mode.
     */
    [[nodiscard]] std::string toFileRestart(const std::string& value) {
        try {
            return QaplaTester::to_string(QaplaTester::parseRestartOption(value));
        }
        catch (const std::exception&) {
            return QaplaHelpers::to_lowercase(value);
        }
    }

    [[nodiscard]] bool isUseFlag(std::string_view lowercaseKey) {
        return lowercaseKey == "usehash" || lowercaseKey == "useponder"
            || lowercaseKey == "usetrace" || lowercaseKey == "userestart";
    }

    /**
     * @brief Lists what an "each" section actually applies to every engine.
     *
     * "id" only scopes the section. A "use..." flag is the GUI's way of remembering a value it
     * does not apply -- an older state file and qapla-chess-gui.ini both carry them. The flag is
     * never itself a setting, and what it switches off is not in force; treating either as one
     * would push GUI-only keys into the engine sections, which the CLI then refuses to read.
     */
    [[nodiscard]] QaplaHelpers::IniFile::KeyValueMap settingsInForce(
        const QaplaHelpers::IniFile::Section& section) {
        QaplaHelpers::IniFile::KeyValueMap settings;
        for (const auto& [key, value] : section.entries) {
            const auto lowercaseKey = QaplaHelpers::to_lowercase(key);
            if (lowercaseKey == "id" || isUseFlag(lowercaseKey)) {
                continue;
            }
            const auto useFlag = useFlagFor(lowercaseKey);
            if (!useFlag.empty() && !isUsed(section, useFlag, true)) {
                continue;
            }
            settings.emplace_back(key, value);
        }
        return settings;
    }

} // namespace

QaplaHelpers::IniFile::Section toTournamentSection(
    const QaplaTester::TournamentConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "tournament";
    section.addEntry("id", id);
    section.addEntry("event", config.event);
    section.addEntry("type", config.type);
    section.addEntry("file", config.tournamentFilename);
    section.addEntry("saveintervalS", std::to_string(config.saveIntervalMs / 1000));
    section.addEntry("games", std::to_string(config.games));
    section.addEntry("rounds", std::to_string(config.rounds));
    section.addEntry("repeat", std::to_string(config.repeat));
    section.addEntry("ratinginterval", std::to_string(config.ratingInterval));
    section.addEntry("outcomeinterval", std::to_string(config.outcomeInterval));
    section.addEntry("averageelo", std::to_string(config.averageElo));
    section.addEntry("noswap", config.noSwap ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toOpeningsSection(
    const QaplaTester::Openings& openings, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "openings";
    section.addEntry("id", id);
    section.addEntry("file", openings.file);
    section.addEntry("order", openings.order);
    section.addEntry("plies", openings.plies ? std::to_string(*openings.plies + 1) : "all");
    section.addEntry("start", std::to_string(openings.start + 1));
    section.addEntry("srand", std::to_string(openings.seed));
    section.addEntry("policy", openings.policy);
    return section;
}

QaplaHelpers::IniFile::Section toPgnOutputSection(
    const QaplaTester::PgnSave::Options& options, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "pgnoutput";
    section.addEntry("id", id);
    section.addEntry("file", options.file);
    section.addEntry("append", options.append ? "true" : "false");
    section.addEntry("finished", options.onlyFinishedGames ? "true" : "false");
    section.addEntry("min", options.minimalTags ? "true" : "false");
    section.addEntry("clock", options.includeClock ? "true" : "false");
    section.addEntry("eval", options.includeEval ? "true" : "false");
    section.addEntry("depth", options.includeDepth ? "true" : "false");
    section.addEntry("pv", options.includePv ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toDrawAdjudicationSection(
    const QaplaTester::AdjudicationManager::DrawAdjudicationConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "draw";
    section.addEntry("id", id);
    section.addEntry("active", config.active ? "true" : "false");
    section.addEntry("movenumber", std::to_string(config.minFullMoves));
    section.addEntry("movecount", std::to_string(config.requiredConsecutiveMoves));
    section.addEntry("score", std::to_string(config.centipawnThreshold));
    section.addEntry("test", config.testOnly ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toResignAdjudicationSection(
    const QaplaTester::AdjudicationManager::ResignAdjudicationConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "resign";
    section.addEntry("id", id);
    section.addEntry("active", config.active ? "true" : "false");
    section.addEntry("movecount", std::to_string(config.requiredConsecutiveMoves));
    section.addEntry("score", std::to_string(config.centipawnThreshold));
    section.addEntry("twosided", config.twoSided ? "true" : "false");
    section.addEntry("test", config.testOnly ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toSprtSection(
    const QaplaTester::SprtConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "sprt";
    section.addEntry("id", id);
    section.addEntry("eloH0", std::to_string(config.eloH0));
    section.addEntry("eloH1", std::to_string(config.eloH1));
    section.addEntry("alpha", std::to_string(config.alpha));
    section.addEntry("beta", std::to_string(config.beta));
    section.addEntry("maxgames", std::to_string(config.maxGames));
    section.addEntry("model", config.model);
    section.addEntry("pentanomial", config.pentanomial ? "true" : "false");
    return section;
}

QaplaHelpers::IniFile::Section toEachSection(
    const QaplaTester::EngineGlobalConfig& config, const std::string& id) {
    QaplaHelpers::IniFile::Section section;
    section.name = "each";
    section.addEntry("id", id);
    if (!config.timeControl.empty()) {
        section.addEntry("tc", config.timeControl);
    }
    if (config.useGlobalHash) {
        section.addEntry("option.Hash", std::to_string(config.hashSizeMB));
    }
    if (config.useGlobalPonder) {
        section.addEntry("ponder", config.ponder ? "true" : "false");
    }
    // The controls label their choices for reading ("None", "Engine decides"); the file format
    // defines them in lower case. Both spellings are accepted on reading, so this is not about
    // being understood - it is about a file that says what the format documents.
    if (config.useGlobalTrace) {
        section.addEntry("trace", QaplaHelpers::to_lowercase(config.traceLevel));
    }
    if (config.useGlobalRestart) {
        section.addEntry("restart", toFileRestart(config.restart));
    }
    return section;
}

QaplaTester::EngineGlobalConfig fromEachSection(const QaplaHelpers::IniFile::Section& section) {
    QaplaTester::EngineGlobalConfig config;
    // "not named" rather than the struct's default, so that a caller can tell a state file that
    // sets no time control from one that happens to set the default one.
    config.timeControl.clear();

    const auto hash = findValue(section, { "option.hash", "hash" });
    config.useGlobalHash = isUsed(section, "usehash", hash.has_value());
    if (hash) {
        config.hashSizeMB = QaplaHelpers::to_unsigned_int<uint32_t>(*hash).value_or(config.hashSizeMB);
    }

    const auto ponder = findValue(section, { "ponder" });
    config.useGlobalPonder = isUsed(section, "useponder", ponder.has_value());
    if (ponder) {
        config.ponder = toBool(*ponder);
    }

    const auto trace = findValue(section, { "trace" });
    config.useGlobalTrace = isUsed(section, "usetrace", trace.has_value());
    if (trace) {
        config.traceLevel = *trace;
    }

    const auto restart = findValue(section, { "restart" });
    config.useGlobalRestart = isUsed(section, "userestart", restart.has_value());
    if (restart) {
        config.restart = *restart;
    }

    if (const auto timeControl = findValue(section, { "tc" })) {
        config.timeControl = *timeControl;
    }

    return config;
}

ResolvedEachDefaults resolveEachDefaults(
    const QaplaHelpers::IniFile::Section& each,
    const QaplaHelpers::IniFile::SectionList& engines) {

    ResolvedEachDefaults resolved{ .each = each, .engines = engines };
    const auto settings = settingsInForce(each);

    // 1. The file's own rule: "each" is what an engine does not say for itself.
    for (const auto& [key, value] : settings) {
        const auto lowercaseKey = QaplaHelpers::to_lowercase(key);
        for (auto& engine : resolved.engines) {
            if (!findValue(engine, { lowercaseKey })) {
                engine.addEntry(key, value);
            }
        }
    }

    // 2. What all engines agree on is what the GUI can offer as a global setting. Rebuilt rather
    //    than edited in place so the entry order of "each" is kept.
    resolved.each.entries.clear();
    if (const auto id = findValue(each, { "id" })) {
        resolved.each.addEntry("id", *id);
    }
    for (const auto& [key, value] : settings) {
        const auto lowercaseKey = QaplaHelpers::to_lowercase(key);
        if (!isGlobalSetting(lowercaseKey)) {
            continue;  // step 1 gave it to every engine; that is where it stays
        }

        const bool agreed = std::ranges::all_of(resolved.engines,
            [&](const QaplaHelpers::IniFile::Section& engine) {
                return findValue(engine, { lowercaseKey }) == value;
            });

        if (agreed) {
            resolved.each.addEntry(key, value);
            for (auto& engine : resolved.engines) {
                eraseKey(engine, lowercaseKey);
            }
            continue;
        }
        // Not global, so the engines keep it. Showing the value as switched off beats dropping
        // it: the user sees what the file proposed and can put it back in force with one click.
        if (const auto useFlag = useFlagFor(lowercaseKey); !useFlag.empty()) {
            resolved.each.addEntry(std::string(useFlag), "false");
            resolved.each.addEntry(key, value);
        }
    }

    return resolved;
}

QaplaHelpers::IniFile::SectionList withoutEachDefaults(
    const QaplaHelpers::IniFile::SectionList& engines,
    const QaplaHelpers::IniFile::Section& each) {

    QaplaHelpers::IniFile::SectionList result = engines;
    for (const auto& [key, _] : settingsInForce(each)) {
        const auto lowercaseKey = QaplaHelpers::to_lowercase(key);
        for (auto& engine : result) {
            eraseKey(engine, lowercaseKey);
        }
    }
    return result;
}

QaplaHelpers::IniFile::SectionList toParticipantSections(
    const std::vector<QaplaTester::EngineConfig>& engines, const std::string& id) {
    QaplaHelpers::IniFile::SectionList sections;
    for (const auto& engine : engines) {
        if (!engine.isSelected()) {
            continue;
        }
        auto section = engine.toSection("engine");
        // toSection() carries over the "id" of the section this engine was read from.
        section.changeOrAddEntry("id", id);
        // toSection() omits "selected" for a selected engine; erase it anyway so a section
        // built from a config with a stale flag cannot carry the GUI-only key into the file.
        section.eraseEntry("selected");
        sections.push_back(std::move(section));
    }
    return sections;
}

} // namespace QaplaConfiguration
