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

/**
 * =============================================================================
 * LINUX IMPLEMENTATION
 * =============================================================================
 * 
 * Uses GTK dialogs with nullptr as parent.
 * 
 * Future improvement: Use XDG Desktop Portal via GDBus (part of GLib, already available via GTK):
 *   1. Call org.freedesktop.portal.FileChooser via D-Bus
 *   2. Portal shows native desktop dialog (GTK on GNOME, Qt on KDE)
 *   3. No parent window needed - Portal handles modality
 *   4. Async by design - fits openFileDialogAsync() API perfectly
 *   5. For sync version: Use GMainLoop to wait for D-Bus response
 *   Reference: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.FileChooser.html
 * 
 * =============================================================================
 */

#ifdef __linux__

#include "os-dialogs.h"
#include "os-helpers.h"
#include "../extern/qapla-engine-tester/src/base-elements/string-helper.h"
#include <vector>
#include <filesystem>
#include <string>
#include <ranges>

#include <gtk/gtk.h>

namespace QaplaWindows {

namespace {

/**
 * @brief Adds one filter to a GTK chooser, built from extensions rather than from a pattern.
 * @param chooser The dialog to add it to.
 * @param extensions Extensions without a dot; empty adds the "All files" entry.
 */
void addFilter(GtkFileChooser* chooser, const std::vector<std::string>& extensions) {
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, OsDialogs::describeFileTypes(extensions).c_str());
    if (extensions.empty()) {
        gtk_file_filter_add_pattern(filter, OsDialogs::fileTypePattern("").c_str());
    }
    for (const auto& extension : extensions) {
        gtk_file_filter_add_pattern(filter, OsDialogs::fileTypePattern(extension).c_str());
    }
    gtk_file_chooser_add_filter(chooser, filter);
}

/**
 * @brief Gives the dialog the filter it was opened for and, unless refused, "All files" beside it.
 */
void addFilters(GtkFileChooser* chooser, const std::vector<std::string>& extensions,
                bool allowNoFilter) {
    if (!extensions.empty()) {
        addFilter(chooser, extensions);
    }
    if (allowNoFilter || extensions.empty()) {
        addFilter(chooser, {});
    }
}

} // namespace

std::vector<std::string> OsDialogs::openFileDialog(const std::vector<std::string>& extensions,
    bool multiple, bool allowNoFilter) {
    std::vector<std::string> results;

    gtk_init(nullptr, nullptr);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Open File", nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        nullptr);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);
    addFilters(GTK_FILE_CHOOSER(dialog), extensions, allowNoFilter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        for (GSList* iter = files; iter != nullptr; iter = iter->next) {
            results.emplace_back((char*)iter->data);
            g_free(iter->data);
        }
        g_slist_free(files);
    }
    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();

    return results;
}

/**
 * Adds the first offered extension to a name that was typed without one.
 * @param path The path the user chose.
 * @param extensions The extensions the dialog offered; empty leaves the path alone.
 * @return The path, with an extension if it had none.
 */
static std::string addExtensionIfMissing(const std::string& path,
                                         const std::vector<std::string>& extensions) {
    if (extensions.empty() || extensions.front().empty()
        || std::filesystem::path(path).has_extension()) {
        return path;
    }
    return path + "." + extensions.front();
}

#include <gtk/gtk.h>
#include <filesystem>
#include <pwd.h>
#include <unistd.h>

std::string OsDialogs::saveFileDialog(const std::vector<std::string>& extensions,
        const std::string& defaultPath, bool allowNoFilter)
{
    gtk_init(nullptr, nullptr);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Save File", nullptr,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        nullptr);

    if (!defaultPath.empty()) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), defaultPath.c_str());
    }

    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    addFilters(GTK_FILE_CHOOSER(dialog), extensions, allowNoFilter);

    std::string result;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            result = filename;
            g_free(filename);
            result = addExtensionIfMissing(result, extensions);
        }
    }
    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();

    return result;
}

std::string OsDialogs::selectFolderDialog(const std::string& defaultPath) {
    gtk_init(nullptr, nullptr);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Select Folder", nullptr,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT,
        nullptr);

    // Set default path if provided
    if (!defaultPath.empty()) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), defaultPath.c_str());
    }

    std::string result;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* foldername = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (foldername) {
            result = foldername;
            g_free(foldername);
        }
    }

    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();
    return result;
}

std::string OsDialogs::getConfigDirectory() {
    // A session started with --config-dir keeps everything it stores in that one directory, so
    // this second way of naming the configuration directory has to answer with it too -- the
    // auto-saved PGN is written through here. Only what the default is stays below.
    auto overrideDirectory = QaplaHelpers::OsHelpers::configDirectoryOverride();
    if (!overrideDirectory.empty()) {
        return overrideDirectory;
    }

    // TODO: Give Linux the same separation macOS already has, once someone can build and run it
    // on Linux. Left untouched on purpose: the change was prepared on a Mac, and touching shared
    // code for one OS is exactly what must not happen here.
    //
    // What macOS looks like now, as the template:
    //   - OsHelpers::getConfigDirectory() for macOS lives in its own file, os-helpers-apple.cpp,
    //     wrapped in #ifdef __APPLE__ (CMake globs src/*.cpp, so the TU is simply empty elsewhere).
    //   - os-helpers.cpp excludes that definition via #ifndef __APPLE__, so its #else branch no
    //     longer serves two operating systems at once. For Linux it still does — that branch is
    //     shared with nothing today only because macOS moved out.
    //   - This function forwards to OsHelpers::getConfigDirectory() instead of computing the path
    //     a second time, leaving one implementation per OS instead of two that can drift apart.
    //
    // Two things to fix while doing it:
    //   - std::getenv -> QaplaHelpers::OsHelpers::getEnv(), the single accessor for environment
    //     variables. No warning on Linux toolchains, but MSVC deprecates std::getenv, so the
    //     codebase uses one accessor everywhere.
    //   - The passwd fallback below is broken: pw_dir points into buffer, and both pwd and buffer
    //     die at the closing brace of the if-block, so the homeDir read afterwards dereferences a
    //     dangling pointer. Only reachable when HOME is unset (systemd units, stripped env).
    //     Copy the value into a std::string that outlives the block, as os-helpers-apple.cpp does.
    const char* homeDir = getenv("HOME");
    
    // Fallback to passwd if HOME not set
    if (homeDir == nullptr) {
        struct passwd pwd;
        struct passwd* result = nullptr;
        char buffer[4096];
        
        if (getpwuid_r(getuid(), &pwd, buffer, sizeof(buffer), &result) == 0 && result != nullptr) {
            homeDir = pwd.pw_dir;
        }
    }
    
    if (homeDir != nullptr) {
        std::filesystem::path configPath(homeDir);
        configPath /= ".qapla-chess-gui";
        return configPath.string();
    }
    
    return "";
}

// ============================================================================
// ASYNC DIALOG IMPLEMENTATIONS
// ============================================================================
// Simply call the synchronous version and pass result to callback.

void OsDialogs::selectFolderDialogAsync(SelectFolderCallback callback,
    const std::string& defaultPath) 
{
    auto result = selectFolderDialog(defaultPath);
    if (callback) {
        callback(result);
    }
}

} // namespace QaplaWindows

#endif // __linux__
