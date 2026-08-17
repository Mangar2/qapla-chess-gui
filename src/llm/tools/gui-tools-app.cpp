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

#include "gui-tools.h"
#include "gui-tools-shared.h"
#include "../actions/gui-action-app.h"
#include "../actions/gui-action-engines.h"

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace QaplaLlm {

namespace {
    using Actions::EngineTarget;
    using Actions::PgnSource;

    /** @brief Arguments of a tool that takes none. */
    struct NoArguments {};

    /** @brief Arguments of open_pgn_file. */
    struct OpenPgnRequest {
        std::optional<PgnSource> source;
    };

    /** @brief The operations manage_engines dispatches to. */
    enum class EngineCommand { List, Details, Install, Copy, Delete, Update, SetOptions };

    /**
     * @brief Arguments of manage_engines -- the union of what its commands need.
     *
     * One tool rather than seven, for the reason the engine-tester's manage_engines is one tool:
     * every command works on the same thing, an engine configuration, and they share most of
     * their arguments. Seven schemas would repeat "which engine" and "which set" seven times over
     * in the part of the prompt that has to stay stable, and a small model picking from a longer
     * list of near-identical tool names does worse than one picking a command out of an enum.
     */
    struct ManageEnginesRequest {
        std::optional<EngineCommand> command;
        std::optional<std::string> engine;
        std::optional<std::string> new_name;
        std::optional<std::string> path;
        std::optional<EngineTarget> target;
        std::vector<std::pair<std::string, std::string>> options;
        std::vector<std::string> unset_options;
        std::vector<std::pair<std::string, std::string>> set;
    };

    /** @brief Carries the model's name/value object into the actions layer's own pair type. */
    std::vector<QaplaLlm::EngineAssignment> assignmentsOf(
        const std::vector<std::pair<std::string, std::string>>& pairs) {
        std::vector<QaplaLlm::EngineAssignment> assignments;
        assignments.reserve(pairs.size());
        for (const auto& [name, value] : pairs) {
            assignments.push_back({.name = name, .value = value});
        }
        return assignments;
    }

} // namespace

void registerAppTools(GuiToolRegistry& registry) {
    Api::defineTool<NoArguments>(registry,
        {.name = "close_application",
            .description = "Closes whole Qapla Chess GUI app (same as window's own close button). "
                           "Immediate -- on close/quit/exit request, call directly, no "
                           "confirmation first.",
            .invoke = [](const NoArguments&) { return Actions::closeApplication(); },
            // Not over the remote control: it would end the very application the caller asked to
            // watch, and take the channel down with it -- see CallOrigin.
            .localOnly = true});

    Api::defineTool<OpenPgnRequest>(registry,
        {.name = "open_pgn_file",
            .description =
                "Opens PGN file in Pgn tab, switches to it. Either user picks file via "
                "native dialog, or directly opens PGN file classic tournament or SPRT "
                "test is currently writing to (their own \"source\" values) -- see "
                "\"source\" param for exactly which. source=\"dialog\" ENDS YOUR TURN "
                "(cancelled or not): that result is the last thing you produce, shown to "
                "the user as-is, no reply_to_user afterwards. source=\"tournament\"/"
                "\"sprt\" open no dialog and are followed by a reply as usual.",
            .params = {Api::enumParam<OpenPgnRequest>("source", &OpenPgnRequest::source,
                "PGN file source. \"dialog\" (default): native file picker, user picks any PGN "
                "file -- use when user names specific file/path you don't know, or just says "
                "\"open a PGN file\" unqualified. \"tournament\": opens PGN file classic "
                "tournament currently writes to (see configure_tournament's pgn_file) -- use for "
                "\"open the PGN file from the tournament\". \"sprt\": same, for SPRT test's own "
                "PGN output (see configure_sprt's pgn_file). Both need no dialog, path already "
                "known.",
                {{"dialog", PgnSource::AskUser}, {"tournament", PgnSource::TournamentOutput},
                    {"sprt", PgnSource::SprtOutput}})},
            .invoke = [](const OpenPgnRequest& request) {
                return Actions::openPgnFile(request.source.value_or(PgnSource::AskUser));
            },
            .timeout = Tools::FILE_DIALOG_TIMEOUT,
            // Its default source is a file dialog, and the two that aren't only switch a tab the
            // remote caller cannot see. Kept local whole rather than split by argument value:
            // a tool that is safe for some values of one parameter and not others is a rule
            // nobody reading the schema would infer.
            .localOnly = true});
}

void registerEngineTools(GuiToolRegistry& registry) {
    Api::defineTool<ManageEnginesRequest>(registry,
        {.name = "manage_engines",
            .description =
                "Everything about the GUI's engine catalog, by \"command\". An engine "
                "configuration is one complete thing -- executable, protocol, time control, UCI "
                "option values -- and it exists in sets: the catalog is the library, and each run "
                "(SPRT, tournament, EPD) holds its own copies of the engines selected for it. "
                "Selecting an engine copies it out of the catalog, so change a run's copy AFTER "
                "selecting, and know that changing the catalog does not reach a run that already "
                "selected it. Commands: \"list\" every catalog engine; \"details\" one engine "
                "with the UCI options its program supports; \"install\" a program from a path; "
                "\"copy\" a catalog entry under a new name; \"delete\" one; \"update\" generic "
                "properties; \"set_options\" UCI option values.",
            .params = {
                Api::enumParam<ManageEnginesRequest>("command", &ManageEnginesRequest::command,
                    "What to do. \"list\": all catalog engines with their protocol -- needs "
                    "nothing else. \"details\": everything about \"engine\", including which UCI "
                    "options its program supports, with types and ranges; read this before "
                    "setting any option. \"install\": adds the program at \"path\" as "
                    "\"new_name\" and starts it to find out what it supports. \"copy\": duplicates "
                    "\"engine\" as \"new_name\", carrying its values over; pass \"options\"/"
                    "\"set\" in the same call to make the copy differ. \"delete\": removes "
                    "\"engine\" from the catalog. \"update\": writes \"set\" (generic properties) "
                    "onto \"engine\" in \"target\". \"set_options\": writes \"options\" and "
                    "clears \"unset_options\" on \"engine\" in \"target\".",
                    {{"list", EngineCommand::List}, {"details", EngineCommand::Details},
                        {"install", EngineCommand::Install}, {"copy", EngineCommand::Copy},
                        {"delete", EngineCommand::Delete}, {"update", EngineCommand::Update},
                        {"set_options", EngineCommand::SetOptions}},
                    true),
                Api::stringParam<ManageEnginesRequest>("engine", &ManageEnginesRequest::engine,
                    "The engine to act on, as \"list\" reports it -- a distinctive part of the "
                    "name is enough. Needed by details, copy (the source), delete, update and "
                    "set_options."),
                Api::stringParam<ManageEnginesRequest>("new_name", &ManageEnginesRequest::new_name,
                    "The catalog name to create: for install, what the program is to be called; "
                    "for copy, the name of the copy. It is how every other tool refers to the "
                    "engine, so make it say which build it is. A name already in the catalog is "
                    "refused."),
                Api::stringParam<ManageEnginesRequest>("path", &ManageEnginesRequest::path,
                    "Full path of the engine executable on disk. For install only. The same "
                    "executable may be installed more than once under different names -- that is "
                    "how one build is tested against itself under different UCI options, since "
                    "the options belong to the catalog entry and not to the file."),
                Api::enumParam<ManageEnginesRequest>("target", &ManageEnginesRequest::target,
                    "Which copy of the engine update and set_options change. \"catalog\" "
                    "(default): the installed engine, which is what future selections start from "
                    "-- it does NOT change a run that already selected it. \"sprt\", "
                    "\"tournament\", \"epd\": the copy that run uses, affecting that run only.",
                    {{"catalog", EngineTarget::Catalog}, {"sprt", EngineTarget::Sprt},
                        {"tournament", EngineTarget::Tournament}, {"epd", EngineTarget::Epd}}),
                Api::stringMapParam<ManageEnginesRequest>("options",
                    &ManageEnginesRequest::options,
                    "UCI option values to set, as an object of name to value, e.g. {\"Hash\": "
                    "\"256\", \"Threads\": \"2\"}. Names must be ones the \"details\" command "
                    "lists as supported; anything else is refused rather than set. Used by "
                    "set_options, and by copy to give the copy its own values."),
                Api::stringListParam<ManageEnginesRequest>("unset_options",
                    &ManageEnginesRequest::unset_options,
                    "UCI options to clear, so the engine uses its own default for them again. "
                    "There is no other way back: leaving an option out of \"options\" keeps "
                    "whatever it already is. For set_options only."),
                Api::stringMapParam<ManageEnginesRequest>("set", &ManageEnginesRequest::set,
                    "Generic engine properties to change, as an object of name to value. "
                    "Settable: cmd (the executable), dir, args, proto (uci/xboard), tc (this "
                    "engine's own time control), trace, restart, ponder, gauntlet, whitepov. NOT "
                    "for UCI options -- those go in \"options\". Used by update, and by copy.")},
            .invoke = [](const ManageEnginesRequest& request) {
                // The dispatch belongs here rather than in the actions: which commands exist and
                // what they are called is an external decision, and the actions stay one function
                // per operation, unaware they are reached through a shared name.
                const auto target = request.target.value_or(EngineTarget::Catalog);
                const auto engine = request.engine.value_or("");
                switch (request.command.value_or(EngineCommand::List)) {
                    case EngineCommand::Details:
                        return Actions::engineDetails(engine);
                    case EngineCommand::Install:
                        return Actions::installEngines({{.name = request.new_name.value_or(""),
                            .path = request.path.value_or("")}});
                    case EngineCommand::Copy:
                        return Actions::copyEngine(engine, request.new_name.value_or(""),
                            assignmentsOf(request.options), assignmentsOf(request.set));
                    case EngineCommand::Delete:
                        return Actions::deleteEngine(engine);
                    case EngineCommand::Update:
                        return Actions::updateEngine(target, engine, assignmentsOf(request.set));
                    case EngineCommand::SetOptions:
                        return Actions::setEngineOptions(target, engine,
                            assignmentsOf(request.options), request.unset_options);
                    case EngineCommand::List:
                    default:
                        return Actions::listInstalledEngines();
                }
            },
            // Installing starts every named engine and waits for it to answer, which is slower
            // than a tool call normally is.
            .timeout = std::chrono::seconds(120)});

    Api::defineTool<NoArguments>(registry,
        {.name = "open_add_engine_dialog",
            .description =
                "Installs a NEW engine program into the GUI's global engine catalog by opening "
                "the GUI's native file picker, so the user selects the executable themselves. Use "
                "this ONLY when you do not know the path -- with a path, manage_engines "
                "command=\"install\" does the same thing without interrupting the user. New "
                "engines are detected (protocol, options) synchronously before the call returns. "
                "ENDS YOUR TURN: this result is the last thing you produce, it is shown to the "
                "user as-is. No further tool call, no reply_to_user afterwards -- you won't be "
                "asked again.",
            .invoke = [](const NoArguments&) { return Actions::addEnginesViaDialog(); },
            .timeout = Tools::FILE_DIALOG_TIMEOUT,
            // The whole tool is a file dialog waiting on the person at the window; there is
            // nothing left of it for a caller who has no window -- see CallOrigin.
            .localOnly = true});
}

// The order is part of the external API, not an implementation detail: it is the order the tools
// appear in for the model, and a small one weights the top of a long list heavily. So the four
// short lifecycle tools come first -- they are what most requests actually want, and they used to
// sit behind ~250 lines of configuration schema. The engine-catalog tools go last, and within them
// the file dialog goes last of all, because it ends the turn -- a wrong reach for it is
// unrecoverable within that turn, while a wrong reach for any of the others is not.
//
// It is also a fixed order, never a computed one: the tool list is the stable head of every
// request's prompt, and reordering it invalidates the cached prefix for every conversation.
void registerGuiTools(GuiToolRegistry& registry) {
    registerActivityTools(registry);
    registerTournamentTools(registry);
    registerSprtTools(registry);
    registerEpdTools(registry);
    registerClopTools(registry);
    registerAppTools(registry);
    registerEngineTools(registry);
}

} // namespace QaplaLlm
