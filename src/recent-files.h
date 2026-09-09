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

#include <cstddef>
#include <string>
#include <vector>

namespace QaplaWindows {

    /**
     * @brief The files last used for one purpose, newest first.
     *
     * One list per purpose, named by an id that is also where it is stored in the configuration:
     * the file an analysis reads and the file it writes are two different questions, and offering
     * one list for both would answer neither.
     */
    class RecentFiles {
    public:
        /** @brief How many files a list keeps. */
        static constexpr size_t MAX_FILES = 5;

        /**
         * @param id Name of the list, in the configuration and nowhere else visible.
         */
        explicit RecentFiles(std::string id);

        /**
         * @brief Puts a file at the front, dropping the oldest once the list is full.
         *
         * A file already in the list moves to the front rather than appearing twice. Saved right
         * away, so a list survives a session that ends unexpectedly.
         *
         * @param path The file that was used. An empty path is ignored.
         */
        void add(const std::string& path);

        /** @brief The files, newest first. */
        [[nodiscard]] const std::vector<std::string>& get() const { return files_; }

        /** @brief Reads the list from the configuration. */
        void loadConfiguration();

        /** @brief Writes the list to the configuration. */
        void updateConfiguration() const;

    private:
        std::string id_;
        std::vector<std::string> files_;
    };

}
