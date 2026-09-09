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

#include "recent-files.h"
#include "configuration.h"

#include <algorithm>
#include <format>
#include <utility>

namespace QaplaWindows {

    namespace {
        constexpr const char* GROUP = "recentfiles";
    }

    RecentFiles::RecentFiles(std::string id) : id_(std::move(id)) {
        loadConfiguration();
    }

    void RecentFiles::add(const std::string& path) {
        if (path.empty()) {
            return;
        }
        std::erase(files_, path);
        files_.insert(files_.begin(), path);
        if (files_.size() > MAX_FILES) {
            files_.resize(MAX_FILES);
        }
        updateConfiguration();
    }

    void RecentFiles::loadConfiguration() {
        files_.clear();
        const auto sections = QaplaConfiguration::Configuration::instance().getConfigData()
            .getSectionList(GROUP, id_);
        if (!sections || sections->empty()) {
            return;
        }
        const auto& section = (*sections)[0];
        for (size_t i = 0; i < MAX_FILES; ++i) {
            const auto value = section.getValue(std::format("file{}", i));
            if (value && !value->empty()) {
                files_.push_back(*value);
            }
        }
    }

    void RecentFiles::updateConfiguration() const {
        QaplaHelpers::IniFile::Section section{ .name = GROUP, .entries = {} };
        section.addEntry("id", id_);
        for (size_t i = 0; i < files_.size(); ++i) {
            section.addEntry(std::format("file{}", i), files_[i]);
        }
        QaplaConfiguration::Configuration::instance().getConfigData()
            .setSectionList(GROUP, id_, { section });
    }

}
