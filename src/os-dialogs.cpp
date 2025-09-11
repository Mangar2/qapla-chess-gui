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

#include "os-dialogs.h"
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>

std::vector<std::string> OsDialogs::openFileDialog(bool multiple) {
    std::vector<std::string> results;
    char filename[32768] = { 0 };

    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (multiple) {
        ofn.Flags |= OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    }

    if (GetOpenFileNameA(&ofn)) {
        char* ptr = filename;
        std::string first = ptr;
        ptr += first.size() + 1;

        if (*ptr == 0) {
            results.push_back(first); // Single file selected
        }
        else {
            std::string folder = first;
            while (*ptr) {
                results.push_back(folder + "\\" + std::string(ptr));
                ptr += strlen(ptr) + 1;
            }
        }
    }

    return results;
}

#elif defined(__APPLE__)
#include <vector>
#include <string>
#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>

#if __LP64__
using NSInteger  = long;
using NSUInteger = unsigned long;
#else
using NSInteger  = int;
using NSUInteger = unsigned int;
#endif

std::vector<std::string> OsDialogs::openFileDialog(bool multiple) {
    std::vector<std::string> result;

    // pool = [[NSAutoreleasePool alloc] init];
    Class NSAutoreleasePool = (Class)objc_getClass("NSAutoreleasePool");
    id pool = ((id(*)(Class, SEL))objc_msgSend)(NSAutoreleasePool, sel_registerName("alloc"));
    pool = ((id(*)(id, SEL))objc_msgSend)(pool, sel_registerName("init"));

    Class NSOpenPanel = (Class)objc_getClass("NSOpenPanel");
    id panel = ((id(*)(Class, SEL))objc_msgSend)(NSOpenPanel, sel_registerName("openPanel"));

    ((void(*)(id, SEL, BOOL))objc_msgSend)(
        panel, sel_registerName("setAllowsMultipleSelection:"), multiple ? (BOOL)1 : (BOOL)0);

    // NSModalResponseOK == 1
    NSInteger r = ((NSInteger(*)(id, SEL))objc_msgSend)(panel, sel_registerName("runModal"));
    if (r == 1) {
        id urls = ((id(*)(id, SEL))objc_msgSend)(panel, sel_registerName("URLs"));
        NSUInteger count = ((NSUInteger(*)(id, SEL))objc_msgSend)(urls, sel_registerName("count"));
        for (NSUInteger i = 0; i < count; ++i) {
            id url  = ((id(*)(id, SEL, NSUInteger))objc_msgSend)(urls, sel_registerName("objectAtIndex:"), i);
            id path = ((id(*)(id, SEL))objc_msgSend)(url, sel_registerName("path"));
            const char* cstr = ((const char*(*)(id, SEL))objc_msgSend)(path, sel_registerName("UTF8String"));
            if (cstr) result.emplace_back(cstr);
        }
    }

    // [pool drain];  (alternativ: release)
    ((void(*)(id, SEL))objc_msgSend)(pool, sel_registerName("drain"));
    return result;
}

#elif defined(__linux__)

#include <gtk/gtk.h>

std::vector<std::string> OsDialogs::openFileDialog(bool multiple) {
    std::vector<std::string> result;

    gtk_init(nullptr, nullptr);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Open File", nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        nullptr);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        for (GSList* iter = files; iter != nullptr; iter = iter->next) {
            result.emplace_back((char*)iter->data);
            g_free(iter->data);
        }
        g_slist_free(files);
    }
    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();

    return result;
}

#else
std::vector<std::string> OsDialogs::openFileDialog(bool) {
    return {};
}
#endif

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

#ifdef _WIN32

std::string OsDialogs::saveFileDialog(const std::vector<std::pair<std::string, std::string>>& filters, 
        const std::string& defaultPath) 
{
    char filename[MAX_PATH] = { 0 };

    std::string filterStr;
    for (const auto& [desc, ext] : filters) {
        filterStr += desc + '\0' + "*." + ext + '\0';
    }
    if (filterStr.empty()) {
        filterStr = "All Files\0*.*\0";
    }
    if (!defaultPath.empty()) {
        std::snprintf(filename, sizeof(filename), "%s", defaultPath.c_str());
    }

    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = filterStr.c_str();
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn)) {
        std::string result = filename;
        result = addExtensionIfMissing(result, filters, ofn.nFilterIndex - 1);
        return result;
    }
    return {};
}

#elif defined(__APPLE__)
#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <filesystem>

#if __LP64__
using NSInteger  = long;
using NSUInteger = unsigned long;
#else
using NSInteger  = int;
using NSUInteger = unsigned int;
#endif

std::string OsDialogs::saveFileDialog(const std::vector<std::pair<std::string, std::string>>& filters, 
        const std::string& defaultPath) 
{
    std::string result;

    Class poolClass = (Class)objc_getClass("NSAutoreleasePool");
    id pool = ((id(*)(Class, SEL))objc_msgSend)(poolClass, sel_registerName("alloc"));
    pool = ((id(*)(id, SEL))objc_msgSend)(pool, sel_registerName("init"));

    Class panelClass = (Class)objc_getClass("NSSavePanel");
    id panel = ((id(*)(Class, SEL))objc_msgSend)(panelClass, sel_registerName("savePanel"));

    if (!filters.empty()) {
        id filterArray = ((id(*)(Class, SEL))objc_msgSend)((Class)objc_getClass("NSMutableArray"), sel_registerName("array"));
        for (const auto& [desc, ext] : filters) {
            id nsExt = ((id(*)(Class, SEL, const char*))objc_msgSend)(
                (Class)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), ext.c_str());
            ((void(*)(id, SEL, id))objc_msgSend)(filterArray, sel_registerName("addObject:"), nsExt);
        }
        ((void(*)(id, SEL, id))objc_msgSend)(panel, sel_registerName("setAllowedFileTypes:"), filterArray);
    }

    if (!defaultPath.empty()) {
        std::filesystem::path path(defaultPath);
        if (path.has_parent_path()) {
            id nsDir = ((id(*)(Class, SEL, const char*))objc_msgSend)((Class)objc_getClass("NSString"),
                        sel_registerName("stringWithUTF8String:"), path.parent_path().string().c_str());
            id dirUrl = ((id(*)(Class, SEL, id))objc_msgSend)((Class)objc_getClass("NSURL"),
                        sel_registerName("fileURLWithPath:"), nsDir);
            ((void(*)(id, SEL, id))objc_msgSend)(panel, sel_registerName("setDirectoryURL:"), dirUrl);
        }
        if (path.has_filename()) {
            id nsName = ((id(*)(Class, SEL, const char*))objc_msgSend)((Class)objc_getClass("NSString"),
                        sel_registerName("stringWithUTF8String:"), path.filename().string().c_str());
            ((void(*)(id, SEL, id))objc_msgSend)(panel, sel_registerName("setNameFieldStringValue:"), nsName);
        }
    }

    NSInteger r = ((NSInteger(*)(id, SEL))objc_msgSend)(panel, sel_registerName("runModal"));
    if (r == 1) {
        id url = ((id(*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
        id path = ((id(*)(id, SEL))objc_msgSend)(url, sel_registerName("path"));
        const char* cstr = ((const char*(*)(id, SEL))objc_msgSend)(path, sel_registerName("UTF8String"));
        if (cstr) {
            result = cstr;
            if (filters.size() == 1 && filters[0].second != "*") {
                result = addExtensionIfMissing(result, filters, 0);
            }
        }
    }

    ((void(*)(id, SEL))objc_msgSend)(pool, sel_registerName("drain"));
    return result;
}

#elif defined(__linux__)
#include <gtk/gtk.h>
#include <filesystem>

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

#else
std::string OsDialogs::saveFileDialog(const std::vector<std::pair<std::string, std::string>>&) {
    return {};
}
#endif

