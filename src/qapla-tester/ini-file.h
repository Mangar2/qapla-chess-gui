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
#include <map>

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
                        currentSection = Section{ 
                            .name=QaplaHelpers::trim(line.substr(1, line.size() - 2)), 
                            .entries={} 
                        };
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
            using SectionMap = std::map<std::string, IniFile::SectionList>; 
        public:

            ConfigData() = default;
            ~ConfigData() = default;

            /**
             * @brief Saves the configuration data to the output stream in INI file format.
             * @param out The output stream to write the configuration data to.
             */
            void save(std::ostream& out) {
                for (const auto& [id, idSectionLists] : sectionTree_) {
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
                sectionTree_.clear();
                auto iniSections = IniFile::load(in);
                for (const auto& section : iniSections) {
                    addSection(section);
                }
            }

            /**
             * @brief Adds a section to the configuration data.
             * If a section with the same name and id already exists, it will be appended to the list.
             * @param section The section to add.
             */
            void addSection(const IniFile::Section& section) {
                auto name = section.name;
                auto idOpt = section.getValue("id");
                auto id = idOpt ? *idOpt : "default";
                if (sectionTree_[name].find(id) == sectionTree_[name].end()) {
                    sectionTree_[name][id] = IniFile::SectionList{};
                }
                sectionTree_[name][id].push_back(section);
            }

            /**
             * @brief Sets a specific section in the configuration data.
             * If a section with the same name and id already exists, it will be replaced.
             * @param section The section to set.
             */
            void setSectionList(const std::string& name, const std::string& id, 
                const IniFile::SectionList& sectionList) {
                if (!id.empty()) {
                    sectionTree_[name][id] = sectionList;
                } else {
                    sectionTree_[name]["default"] = sectionList;
                }
            }

            /**
             * @brief Retrieves all sections with the given name.
             * @param name The name of the sections to retrieve.
             * @return An optional containing a map of section ids to their corresponding section lists,
             *         or std::nullopt if no sections with the given name exist.
             */
            std::optional<SectionMap> getSectionMap(const std::string& name) const {
                auto it = sectionTree_.find(name);
                if (it != sectionTree_.end()) {
                    return it->second;
                }
                return std::nullopt;    
            }

            /**
             * @brief Retrieves a specific section from the configuration data.
             * @param name The name of the section to retrieve.
             * @param id The identifier of the section to retrieve (default is "default").
             * @return An optional containing the section if found, or std::nullopt if not found.
             */
            std::optional<IniFile::SectionList> getSectionList(const std::string& name, const std::string& id = "default") const {
                auto sectionMapOpt = getSectionMap(name);
                if (!sectionMapOpt) return std::nullopt;
                auto it = sectionMapOpt->find(id);
                if (it != sectionMapOpt->end()) {
                    return it->second;
                }
                return std::nullopt;
            }

        private:

            using SectionTree = std::unordered_map<std::string, SectionMap>;
            SectionTree sectionTree_;
    };

} // namespace QaplaHelpers
