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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include <catch2/catch_test_macros.hpp>

#include "autosavable.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

namespace fs = std::filesystem;

/**
 * @brief A stored file whose content is read only as far as the test tells it to.
 */
class TestStore : public QaplaHelpers::Autosavable {
public:
    explicit TestStore(const std::string& directory)
        : QaplaHelpers::Autosavable("store.ini", ".bak", 60000, [directory]() { return directory; }) {}

    bool failWholeLoad = false;      ///< loadData() throws, as on an unparsable file
    bool failOneSection = false;     ///< One section is skipped, the rest is kept
    std::string loaded;              ///< What loadData() took over
    std::string toWrite = "written"; ///< What saveData() puts into the file

protected:
    void saveData(std::ofstream& out) override {
        out << toWrite;
    }

    void loadData(std::ifstream& in) override {
        std::stringstream buffer;
        buffer << in.rdbuf();
        if (failWholeLoad) {
            throw std::runtime_error("unreadable");
        }
        loaded = buffer.str();
        if (failOneSection) {
            markLoadIncomplete("section [engine] could not be read");
        }
    }
};

/**
 * @brief A directory that cleans itself up, holding a store file with known content.
 */
class TempStore {
public:
    TempStore() : directory_(fs::temp_directory_path() / "qapla-autosavable-guard-test") {
        fs::remove_all(directory_);
        fs::create_directories(directory_);
        std::ofstream out(path());
        out << stored;
    }
    ~TempStore() { fs::remove_all(directory_); }

    TempStore(const TempStore&) = delete;
    TempStore& operator=(const TempStore&) = delete;

    [[nodiscard]] std::string directory() const { return directory_.string(); }
    [[nodiscard]] fs::path path() const { return directory_ / "store.ini"; }
    [[nodiscard]] std::string content() const {
        std::ifstream in(path());
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    static constexpr const char* stored = "everything the user ever configured";

private:
    fs::path directory_;
};

} // namespace

TEST_CASE("A file that was not read completely is not written over", "[autosavable]") {
    // Saving renames the stored file away and deletes it once the new one is written, so it
    // writes out exactly what was read. After an incomplete load that is a fraction of the
    // file -- which is how a single unreadable section once emptied a whole configuration.

    SECTION("Nothing could be read") {
        TempStore store;
        TestStore subject(store.directory());
        subject.failWholeLoad = true;

        subject.loadFile();
        subject.saveFile();

        CHECK(store.content() == TempStore::stored);
    }

    SECTION("Part of it could not be read") {
        TempStore store;
        TestStore subject(store.directory());
        subject.failOneSection = true;

        subject.loadFile();
        subject.saveFile();

        CHECK(subject.loaded == TempStore::stored);  // the readable part is still usable
        CHECK(store.content() == TempStore::stored); // but it is not what gets written back
        CHECK_FALSE(subject.getLoadIncompleteReason().empty());
    }

    SECTION("A file read completely is written as usual") {
        TempStore store;
        TestStore subject(store.directory());

        subject.loadFile();
        subject.saveFile();

        CHECK(store.content() == subject.toWrite);
        CHECK(subject.getLoadIncompleteReason().empty());
    }

    SECTION("A store with no file yet is written as usual") {
        // Nothing exists, so nothing can be lost -- a first start must still save.
        TempStore store;
        fs::remove(store.path());
        TestStore subject(store.directory());

        subject.loadFile();
        subject.saveFile();

        CHECK(store.content() == subject.toWrite);
    }
}
