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

#include <string_view>

namespace QaplaApp {

/**
 * @brief The released version, the same number qapla-engine-tester carries.
 *
 * Kept equal on purpose rather than counted up on its own: the GUI is built on the engine
 * tester's sources and writes the files it reads, so the two are only ever delivered as a
 * matching pair. One number for both says which pair a user has, and a GUI whose number differs
 * from the tester beside it would say nothing at all about whether their files fit together.
 *
 * Raise it here and in `Logger::getWelcomeMessage()` (extern/qapla-engine-tester) together.
 */
inline constexpr std::string_view VERSION = "0.6.0";

} // namespace QaplaApp
