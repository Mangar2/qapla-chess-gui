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
                    out << key << "=" << value << "\n";
                }
                out << "\n";
            }

            /**
             * @brief Saves multiple INI file sections to the output stream.
             *
             * @param out The output stream to write to.
             * @param sections The list of sections to save.
             */
            static void saveSections(std::ostream& out, const SectionList& sections) {
                for (const auto& section : sections) {
                    saveSection(out, section);
                }
            }

    };

    class ConfigData {
        public:
            ConfigData() = default;
            ~ConfigData() = default;

            /**
             * @brief Saves the configuration data to the output stream in INI file format.
             * @param out The output stream to write the configuration data to.
             */
            void save(std::ostream& out) {
                for (const auto& [id, idSectionLists] : sections_) {
                    for (const auto& [sectionId, sectionList] : idSectionLists) {
                        IniFile::saveSections(out, sectionList);
                    }
                }
            }

            /**
             * @brief Loads the configuration data from a list of INI file sections.
             * @param sections The list of sections to load the configuration data from.
             */
            void load(std::istream& in) {
                sections_.clear();
                auto iniSections = IniFile::load(in);
                for (const auto& section : iniSections) {
                    auto name = section.name;
                    auto idOpt = section.getValue("id");
                    auto id = idOpt ? *idOpt : "default";
                    // is a list, make sure it has entries
                    if (sections_[name].find(id) == sections_[name].end()) {
                        sections_[name][id] = IniFile::SectionList{};
                    }
                    sections_[name][id].push_back(section);
                }
            }

            /**
             * @brief Sets a specific section in the configuration data.
             * If a section with the same name and id already exists, it will be replaced.
             * @param section The section to set.
             */
            void setSectionList(const std::string& name, const std::string& id, 
                const IniFile::SectionList& sectionList) {
                if (!id.empty()) {
                    sections_[name][id] = sectionList;
                } else {
                    sections_[name]["default"] = sectionList;
                }
            }

            /**
             * @brief Retrieves a specific section from the configuration data.
             * @param name The name of the section to retrieve.
             * @param id The identifier of the section to retrieve (default is "default").
             * @return An optional containing the section if found, or std::nullopt if not found.
             */
            std::optional<IniFile::SectionList> getSectionList(const std::string& name, const std::string& id = "default") const {
                auto it = sections_.find(name);
                if (it != sections_.end()) {
                    auto idIt = it->second.find(id);
                    if (idIt != it->second.end()) {
                        return idIt->second;
                    }
                }
                return std::nullopt;
            }

        private:
            using IdSections = std::unordered_map<std::string, IniFile::SectionList>;
            using SectionList = std::unordered_map<std::string, IdSections>;
            SectionList sections_;
    };

} // namespace QaplaHelpers
