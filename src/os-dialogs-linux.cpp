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
#include "../extern/qapla-engine-tester/src/string-helper.h"
#include <vector>
#include <filesystem>
#include <string>
#include <ranges>

#include <gtk/gtk.h>

namespace QaplaWindows {

std::vector<std::string> OsDialogs::openFileDialog(bool multiple,
    const std::vector<std::pair<std::string, std::string>>& filters) {
    std::vector<std::string> results;

    gtk_init(nullptr, nullptr);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Open File", nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        nullptr);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);

    // Set file type filters if provided
    for (const auto& filter : filters) {
        GtkFileFilter* gtkFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(gtkFilter, filter.first.c_str());
        gtk_file_filter_add_pattern(gtkFilter, filter.second.c_str());
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtkFilter);
    }

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
 * Adds the appropriate extension to the file path if missing based on the selected filter.
 * @param path The original file path.
 * @param filters The list of filters as pairs of (description, pattern).
 * @param selectedIndex The index of the selected filter.
 * @return The file path with the appropriate extension added if it was missing.
 */
static std::string addExtensionIfMissing(const std::string& path,
                                         const std::vector<std::pair<std::string, std::string>>& filters,
                                         size_t selectedIndex) {
    if (filters.empty() || selectedIndex >= filters.size()) {
        return path;
    }
    const std::string& ext = filters[selectedIndex].second;
    if (ext.empty() || ext == "*" || std::filesystem::path(path).has_extension()) {
        return path;
    }
    return path + "." + ext;
}

#include <gtk/gtk.h>
#include <filesystem>
#include <pwd.h>
#include <unistd.h>

std::string OsDialogs::saveFileDialog(const std::vector<std::pair<std::string, std::string>>& filters, 
        const std::string& defaultPath) 
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

    for (const auto& [desc, ext] : filters) {
        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, desc.c_str());
        std::string pattern = "*." + ext;
        gtk_file_filter_add_pattern(filter, pattern.c_str());
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    }

    std::string result;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            result = filename;
            if (filters.size() == 1 && filters[0].second != "*") {
                result = addExtensionIfMissing(result, filters, 0);
            }
            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();
    return result;
}

// ============================================================================
// FOLDER SELECTION DIALOG
// ============================================================================

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
    // Try environment variable first
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

void OsDialogs::openFileDialogAsync(OpenFileCallback callback,
    bool multiple,
    const std::vector<std::pair<std::string, std::string>>& filters) 
{
    auto result = openFileDialog(multiple, filters);
    if (callback) {
        callback(result);
    }
}

void OsDialogs::saveFileDialogAsync(SaveFileCallback callback,
    const std::vector<std::pair<std::string, std::string>>& filters,
    const std::string& defaultPath) 
{
    auto result = saveFileDialog(filters, defaultPath);
    if (callback) {
        callback(result);
    }
}

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
