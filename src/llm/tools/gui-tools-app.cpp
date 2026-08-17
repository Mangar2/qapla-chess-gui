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

    /** @brief Arguments of install_engines. */
    struct InstallEnginesRequest {
        std::vector<std::pair<std::string, std::string>> engines;
    };

    /** @brief Arguments of get_engine_details. */
    struct EngineDetailsRequest {
        std::optional<std::string> engine;
    };

    /** @brief Arguments of set_engine_options. */
    struct SetEngineOptionsRequest {
        std::optional<std::string> engine;
        std::optional<EngineTarget> target;
        std::vector<std::pair<std::string, std::string>> options;
    };
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
    Api::defineTool<NoArguments>(registry,
        {.name = "list_installed_engines",
            .description = "Lists all chess engines configured in GUI's global engine catalog, "
                           "name + protocol (uci/xboard). Use it when the user asks which engines "
                           "are available, or wants to pick one from a list. To set engines for a "
                           "run just pass the name the user said to configure_tournament/"
                           "configure_sprt/configure_epd -- they match names themselves.",
            .invoke = [](const NoArguments&) { return Actions::listInstalledEngines(); }});

    Api::defineTool<EngineDetailsRequest>(registry,
        {.name = "get_engine_details",
            .description =
                "Reports everything about ONE installed engine: where its executable is, its "
                "protocol, time control, the UCI option values currently set on it, and the full "
                "list of options its program supports -- each with its type, default and allowed "
                "range or choices. Call this before set_engine_options: the supported list is the "
                "only place option names and legal values come from, because every engine reports "
                "its own, and they change from build to build.",
            .params = {Api::stringParam<EngineDetailsRequest>("engine",
                &EngineDetailsRequest::engine,
                "Name of the engine, as list_installed_engines reports it. A distinctive part of "
                "the name is enough.",
                true)},
            .invoke = [](const EngineDetailsRequest& request) {
                return Actions::engineDetails(request.engine.value_or(""));
            }});

    Api::defineTool<InstallEnginesRequest>(registry,
        {.name = "install_engines",
            .description =
                "Installs engine programs into the GUI's global engine catalog from their paths "
                "on disk, and starts each one to find out what it supports (protocol and UCI "
                "options) before returning -- so the result already says what can be configured, "
                "and get_engine_details works straight afterwards. You choose the catalog name "
                "for each: that name is how every other tool refers to the engine. The SAME "
                "executable may be installed twice under two names -- that is how one build is "
                "tested against itself with different UCI options, since options belong to the "
                "catalog entry, not to the file. A name already in the catalog is refused.",
            .params = {Api::stringMapParam<InstallEnginesRequest>("engines",
                &InstallEnginesRequest::engines,
                "The engines to install, as an object of catalog name to full executable path, "
                "e.g. {\"Qapla baseline\": \"/home/me/build/qapla\", \"Qapla candidate\": "
                "\"/home/me/build/qapla-new\"}.",
                true)},
            .invoke = [](const InstallEnginesRequest& request) {
                std::vector<QaplaLlm::NamedEnginePath> engines;
                engines.reserve(request.engines.size());
                for (const auto& [name, path] : request.engines) {
                    engines.push_back({.name = name, .path = path});
                }
                return Actions::installEngines(engines);
            },
            // Detection starts every engine and waits for it to answer, which is slower than a
            // tool call normally is and slower still for several at once.
            .timeout = std::chrono::seconds(120)});

    Api::defineTool<SetEngineOptionsRequest>(registry,
        {.name = "set_engine_options",
            .description =
                "Sets UCI option values (Hash, Threads, tuning parameters, ...) on one engine. "
                "Get the option names and their legal values from get_engine_details first; "
                "anything the engine does not support is refused rather than set. \"target\" "
                "decides WHICH copy of the engine is changed -- an engine selected for a run is a "
                "copy of the catalog entry, so setting options on the run does not touch the "
                "catalog and vice versa. For a run, select the engines FIRST (configure_sprt, "
                "configure_tournament, configure_epd) and set the options afterwards: selecting "
                "takes a fresh copy from the catalog. That is also how the same program plays "
                "against itself under two option sets -- install it twice under different names, "
                "then give each copy its own values.",
            .params = {
                Api::stringParam<SetEngineOptionsRequest>("engine",
                    &SetEngineOptionsRequest::engine,
                    "Name of the engine, as list_installed_engines reports it. A distinctive part "
                    "of the name is enough.",
                    true),
                Api::enumParam<SetEngineOptionsRequest>("target", &SetEngineOptionsRequest::target,
                    "Which copy of the engine to change. \"sprt\", \"tournament\" or \"epd\": the "
                    "copy that run uses, affecting that run only. \"catalog\": the installed "
                    "engine itself, which is what future selections start from -- it does NOT "
                    "change a run that has already selected it.",
                    {{"sprt", EngineTarget::Sprt}, {"tournament", EngineTarget::Tournament},
                        {"epd", EngineTarget::Epd}, {"catalog", EngineTarget::Catalog}},
                    true),
                Api::stringMapParam<SetEngineOptionsRequest>("options",
                    &SetEngineOptionsRequest::options,
                    "The option values to set, as an object of name/value pairs, e.g. "
                    "{\"Hash\": \"256\", \"Threads\": \"2\"}. Names must be ones "
                    "get_engine_details lists as supported. Options left out keep whatever they "
                    "are; there is no way to unset one back to the engine default here.",
                    true)},
            .invoke = [](const SetEngineOptionsRequest& request) {
                std::vector<QaplaLlm::EngineOptionAssignment> assignments;
                assignments.reserve(request.options.size());
                for (const auto& [name, value] : request.options) {
                    assignments.push_back({.name = name, .value = value});
                }
                return Actions::setEngineOptions(
                    request.target.value_or(EngineTarget::Catalog),
                    request.engine.value_or(""), assignments);
            }});

    Api::defineTool<NoArguments>(registry,
        {.name = "open_add_engine_dialog",
            .description =
                "Installs a NEW engine program into the GUI's global engine catalog, one "
                "that is not listed there yet: opens the GUI's native file picker so the "
                "user can select engine executables on disk. This is the only thing it "
                "does. Engines for a tournament, SPRT test or EPD analysis are chosen by "
                "name in configure_tournament/configure_sprt/configure_epd, out of the "
                "engines already installed -- so use this one only when the user says they "
                "want to add or install an engine the catalog doesn't have. User picks "
                "file(s) themselves -- you have no filesystem access. New engines detected "
                "(protocol, options, etc.) synchronously before call returns -- result "
                "already reflects final outcome, don't promise separate future update. "
                "ENDS YOUR TURN: this result is the last thing you produce, it is shown "
                "to the user as-is. No further tool call, no reply_to_user afterwards -- "
                "you won't be asked again.",
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
    registerAppTools(registry);
    registerEngineTools(registry);
}

} // namespace QaplaLlm
