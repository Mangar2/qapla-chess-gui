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
 * @author Volker B�hm
 * @copyright Copyright (c) 2025 Volker B�hm
 */

#include "os-dialogs.h"
#include <vector>

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
