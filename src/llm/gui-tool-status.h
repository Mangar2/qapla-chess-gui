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

#include "gui-tool-registry.h"

namespace QaplaLlm {

/**
 * @brief Registers the "get_running_status" tool: a single combined check across every
 * long-running activity the chat can control (currently tournament mode and SPRT), so a
 * question like "is anything running?" or "is a tournament running?" doesn't get answered from
 * only one of them.
 *
 * Tournament mode and SPRT are separate, independently-running activities (see
 * gui-tool-tournament.h / gui-tool-sprt.h) -- but users often call an SPRT test a "tournament"
 * informally, since to a person running one it looks and feels the same (engines playing games
 * in the background). Answering "is a tournament running?" using only get_tournament_status
 * would incorrectly say "no" while an SPRT test is actively running games. This tool exists so
 * the model has one place to check both and report accurately.
 */
void registerStatusTools(GuiToolRegistry& registry);

} // namespace QaplaLlm
