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
#include <stdexcept>
#include <vector>
#include <optional>
#include <utility>
#include <unordered_map>

#include "string-helper.h"

namespace QaplaHelpers {

    class IniFile {
        public:
            using KeyValueMap = std::vector<std::pair<std::string, std::string>>;
            struct Section {
                std::string name;
                KeyValueMap entries;
                void addEntry(const std::string& key, const std::string& value) {
                    entries.push_back({ key, value });
                }   
                void insertFirst(const std::string& key, const std::string& value) {
                    entries.insert(entries.begin(), { key, value });
                }
                std::optional<std::string> getValue(const std::string& key) const {
                    for (const auto& [k, v] : entries) {
                        if (k == key) return v;
                    }
                    return std::nullopt;
                }
                std::unordered_map<std::string, std::string> getUnorderedMap() const {
                    std::unordered_map<std::string, std::string> map;
                    for (const auto& [k, v] : entries) {
                        map[k] = v;
                    }
                    return map;
                }
            };
            using SectionList = std::vector<Section>;

            /**
             * @brief Loads the INI file sections from the input stream.
             *
             * @param in The input stream to read from.
             * @return SectionList A list of sections parsed from the INI file.
             */
            static SectionList load(std::istream& in) {
                SectionList sections;
                std::optional<Section> currentSection;

                std::string line;
                while (std::getline(in, line)) {
                    line = QaplaHelpers::trim(line);
                    if (line.empty() || line[0] == '#' || line[0] == ';') continue;

                    if (line.front() == '[' && line.back() == ']') {
                        if (currentSection) {
                            sections.push_back(std::move(*currentSection));
                        }
                        currentSection = Section{ QaplaHelpers::trim(line.substr(1, line.size() - 2)), {} };
                    } else {
                        auto keyValue = QaplaHelpers::parseKeyValue(line);
                        if (keyValue && currentSection) {
                            currentSection->addEntry(keyValue->first, keyValue->second);
                        }
                    }
                }
                if (currentSection) {
                    sections.push_back(std::move(*currentSection));
                }
                return sections;
            }

            /**
             * @brief Saves an INI file section to the output stream.
             *
             * @param out The output stream to write to.
             * @param section The section to save.
             */
            static void saveSection(std::ostream& out, const Section& section) {
                out << "[" << section.name << "]\n";
                for (const auto& [key, value] : section.entries) {
                    out << key << " = " << value << "\n";
                }
                out << "\n";
            }

    };

} // namespace QaplaHelpers
