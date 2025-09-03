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

 /**
  * Native file open dialog.
  * Returns a list of selected file paths as std::vector<std::string>.
  * If the dialog is cancelled, the returned vector is empty.
  */
class OsDialogs {
public:
    /**
     * Opens a native file dialog for selecting files.
     *
     * @param multiple If true, allows selecting multiple files.
     * @return Vector of selected file paths, empty if cancelled.
     */
    static std::vector<std::string> openFileDialog(bool multiple = false);
};
