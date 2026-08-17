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

#include <engine-handling/engine-capabilities.h>
#include <engine-handling/engine-config.h>

#include <string>
#include <utility>
#include <vector>

/**
 * @file
 * @brief Engine catalog logic, free of both tool layers and of the GUI stack.
 *
 * Same reasoning as gui-tool-tournament.h: pure enough for the unit-tests target to link
 * directly, so it stays out of src/llm/actions/ (which is GUI-only) and out of src/llm/tools/
 * (which is model-facing only).
 */

namespace QaplaLlm {

/**
 * @brief Returns the globally configured engines (name + protocol) as a JSON array string.
 *
 * Pure/UI-independent so it can be unit-tested directly; this is exactly what the
 * listInstalledEngines() action reports.
 */
[[nodiscard]] std::string listInstalledEnginesJson();

/**
 * @brief Outcome of adding engines from a set of executable paths.
 */
struct AddEnginesOutcome {
    std::vector<std::string> addedNames;
    std::vector<std::string> duplicateNames; ///< Already present (same cmd + protocol); left untouched.
};

/**
 * @brief Adds engines from the given executable paths to the global engine
 * catalog, skipping ones already configured (same cmd + protocol).
 *
 * Pure/UI-independent so it can be unit-tested with fabricated paths (no
 * filesystem access is required to build an EngineConfig from a path); the
 * "open_add_engine_dialog" tool handler is a thin wrapper that only adds
 * the native file picker on top of this.
 */
[[nodiscard]] AddEnginesOutcome addEnginesFromPaths(const std::vector<std::string>& paths);

/** @brief One engine to install: the executable, and what it is to be called in the catalog. */
struct NamedEnginePath {
    std::string name;
    std::string path;
};

/**
 * @brief Outcome of adding engines that were named by the caller.
 */
struct AddNamedEnginesOutcome {
    std::vector<std::string> addedNames;
    std::vector<std::string> takenNames; ///< Names already used in the catalog; nothing was added for these.
};

/**
 * @brief Adds engines under caller-chosen names, refusing only a name that is already taken.
 *
 * Two differences from addEnginesFromPaths(), and both follow from the name being given rather
 * than derived:
 *
 * The same executable may be added more than once, under different names. That is not an accident
 * to be guarded against but the point: an SPRT of one build against itself under two option sets
 * needs two catalog entries pointing at the same file, since option values belong to the
 * configuration and the configuration is keyed by name. Deduplicating by executable -- which is
 * right when the name is only a filename, and is what addEnginesFromPaths() does for the file
 * dialog -- would make that impossible to express.
 *
 * And the name survives detection. An engine that reports its own name replaces a name that is
 * still just the executable's filename (see EngineCapabilities::storeCapabilities and
 * EngineConfig::hasDefaultName), which is helpful for a file picked in a dialog and destructive
 * here: two builds of one engine would both come back named after the engine, leaving neither
 * selectable by name. A name the caller chose is never a default name, so detection leaves it be.
 */
[[nodiscard]] AddNamedEnginesOutcome addNamedEngines(const std::vector<NamedEnginePath>& engines);

/**
 * @brief Reports one engine configuration together with what its program can actually be told.
 *
 * Three things, and they come from two different places on purpose. The configuration's own
 * properties and its set option values belong to *this* configuration, keyed by name. The list of
 * supported options belongs to the executable, keyed by cmd+protocol (see
 * EngineCapabilities::makeKey) -- which is why two configurations of the same build share one
 * option list and can still hold different values for it.
 *
 * The supported options are reported with their full domain -- type, default, range, choices --
 * not just their names. A caller setting values out of a CLOP result has no other way to tell
 * whether what it is about to set is even in range, and an engine handed an out-of-range value is
 * free to ignore it silently, which turns into a test result that measures nothing.
 *
 * @param capabilities Passed in rather than reached for, so this stays free of the GUI singleton
 *        and can be unit-tested with a hand-built capability set.
 */
[[nodiscard]] std::string engineDetailsText(const QaplaTester::EngineConfig& config,
    const QaplaConfiguration::EngineCapabilities& capabilities);

/** @brief One requested UCI option value, as the caller named it. */
struct EngineAssignment {
    std::string name;
    std::string value;
};

/**
 * @brief What applyEngineOptions() did, in the three outcomes worth telling apart.
 */
struct ApplyOptionsOutcome {
    /**
     * @brief False when the executable has never been started, so nothing is known about it.
     *
     * Distinct from "the option does not exist": here the question could not be asked at all, and
     * the answer is to run detection, not to pick a different option name.
     */
    bool detected = true;

    std::vector<std::string> applied;  ///< "Hash = 128", using the engine's own spelling.
    std::vector<std::string> unknown;  ///< Names this engine does not offer.
    std::vector<std::string> rejected; ///< Values outside the option's declared domain, with it named.
};

/**
 * @brief Sets UCI option values on one engine configuration, checked against its capability list.
 *
 * Checked rather than stored blindly, because an engine silently ignores what it does not
 * understand: a misspelt option name or an out-of-range value would otherwise produce a test that
 * ran to completion and measured the wrong build. Every rejection names the domain it violated, so
 * the caller can correct it without a second lookup.
 *
 * Accepted values are stored under the engine's own spelling of the option name, not the caller's:
 * matching is case-insensitive, but what goes to the engine is what the engine called it.
 */
[[nodiscard]] ApplyOptionsOutcome applyEngineOptions(QaplaTester::EngineConfig& config,
    const std::vector<EngineAssignment>& assignments,
    const QaplaConfiguration::EngineCapabilities& capabilities);

/**
 * @brief Removes option values so those options fall back to the engine's own default.
 *
 * The counterpart applyEngineOptions() has no way to express: an option carries no "unset" value,
 * it is either configured or it is not, and a configuration that has been through several tuning
 * rounds needs a way back to the untouched state. Names are matched case-insensitively, as
 * everywhere else, and one that was never set is reported rather than silently accepted -- a
 * caller clearing what it believes it set wants to hear that its belief was wrong.
 *
 * @return The names actually removed, in the engine's own spelling.
 */
[[nodiscard]] std::vector<std::string> unsetEngineOptions(QaplaTester::EngineConfig& config,
    const std::vector<std::string>& names);

/**
 * @brief The generic engine keys a caller may set, in the spelling the ini file and CLI use.
 *
 * A closed list, and it has to be: EngineConfig::setValue() treats any key it does not recognise
 * as a UCI option written without its prefix -- which is right when reading an engines.ini that
 * predates the prefix rule, and wrong here, where a misspelt "protocol" would silently become an
 * option named "protocol" that no engine ever asked for.
 */
[[nodiscard]] const std::vector<std::string>& settableEngineKeys();

/**
 * @brief Sets generic engine properties (cmd, tc, proto, ...) on one configuration.
 *
 * Same reporting split as applyEngineOptions(), for the same reason: a key this configuration has
 * no place for, and a value the key will not take, are different mistakes with different fixes.
 * Values are validated by the accessors themselves -- setProtocol() and setTimeControl() reject
 * what they cannot parse -- and their complaint is caught per key, so one bad value does not cost
 * the caller the rest of the call.
 */
[[nodiscard]] ApplyOptionsOutcome applyEngineProperties(
    QaplaTester::EngineConfig& config, const std::vector<EngineAssignment>& assignments);

/** @brief Outcome of copying a catalog entry. Both flags false means it was copied. */
struct CopyEngineOutcome {
    bool sourceMissing = false;
    bool nameTaken = false;
};

/**
 * @brief Copies a catalog entry, values and all, under a new name.
 *
 * The cheapest way to get a second configuration of one build, and therefore the normal way to
 * vary options: the copy shares the executable, so it needs no detection, and it starts from the
 * source's values rather than from the engine's defaults -- which is what makes "same as the
 * baseline but with one parameter moved" a single step. See addNamedEngines() for why sharing an
 * executable across catalog entries is intended rather than a duplicate to be prevented.
 */
[[nodiscard]] CopyEngineOutcome copyCatalogEngine(
    const std::string& sourceName, const std::string& newName);

/** @brief Removes a catalog entry. @return false if there was no such entry. */
[[nodiscard]] bool deleteCatalogEngine(const std::string& name);

} // namespace QaplaLlm
