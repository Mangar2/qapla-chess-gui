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

namespace QaplaLlm {

namespace {
    using Actions::PgnSource;

    /** @brief Arguments of a tool that takes none. */
    struct NoArguments {};

    /** @brief Arguments of open_pgn_file. */
    struct OpenPgnRequest {
        std::optional<PgnSource> source;
    };
} // namespace

void registerAppTools(GuiToolRegistry& registry) {
    Api::defineTool<NoArguments>(registry,
        {.name = "close_application",
            .description = "Closes whole Qapla Chess GUI app (same as window's own close button). "
                           "Immediate -- on close/quit/exit request, call directly, no "
                           "confirmation first.",
            .invoke = [](const NoArguments&) { return Actions::closeApplication(); }});

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
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

void registerEngineTools(GuiToolRegistry& registry) {
    Api::defineTool<NoArguments>(registry,
        {.name = "list_installed_engines",
            .description = "Lists all chess engines configured in GUI's global engine catalog, "
                           "name + protocol (uci/xboard). Only needed when the user asks what is "
                           "available -- the configure_* tools match names themselves, so never "
                           "call this just to look one up before selecting engines.",
            .invoke = [](const NoArguments&) { return Actions::listInstalledEngines(); }});

    Api::defineTool<NoArguments>(registry,
        {.name = "open_add_engine_dialog",
            .description =
                "Opens GUI's native file picker, user selects one+ chess engine "
                "executables to add to global engine catalog. User picks file(s) "
                "themselves -- you have no filesystem access. New engines detected "
                "(protocol, options, etc.) synchronously before call returns -- result "
                "already reflects final outcome, don't promise separate future update. "
                "ENDS YOUR TURN: this result is the last thing you produce, it is shown "
                "to the user as-is. No further tool call, no reply_to_user afterwards -- "
                "you won't be asked again.",
            .invoke = [](const NoArguments&) { return Actions::addEnginesViaDialog(); },
            .timeout = Tools::FILE_DIALOG_TIMEOUT});
}

void registerGuiTools(GuiToolRegistry& registry) {
    registerEngineTools(registry);
    registerTournamentTools(registry);
    registerSprtTools(registry);
    registerEpdTools(registry);
    registerActivityTools(registry);
    registerAppTools(registry);
}

} // namespace QaplaLlm
