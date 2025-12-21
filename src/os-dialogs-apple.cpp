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
 * PLATFORM-SPECIFIC IMPLEMENTATION NOTES
 * =============================================================================
 * 
 * MACOS (implemented):
 * - Current: Uses runModal which shows dialog as independent modal window
 * - Problem: Dialog can slip behind other applications
 * - Solution: Use Sheet-Dialogs that attach to the parent window:
 *   1. Get NSWindow* via glfwGetCocoaWindow(glfwGetCurrentContext())
 *   2. Use [panel beginSheetModalForWindow:parent completionHandler:^(...){...}]
 *   3. This is async - fits perfectly with openFileDialogAsync() API
 *   4. For sync version: Use [NSApp runModalForWindow:] + [NSApp stopModal] in callback
 *      to block until dialog closes
 * 
 * =============================================================================
 */

#ifdef __APPLE__

#include "os-dialogs.h"
#include "../extern/qapla-engine-tester/src/string-helper.h"
#include <vector>
#include <filesystem>
#include <string>
#include <ranges>

// GLFW for getting native window handle
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <pwd.h>
#include <unistd.h>

#if __LP64__
using NSInteger  = long;
using NSUInteger = unsigned long;
#else
using NSInteger  = int;
using NSUInteger = unsigned int;
#endif

namespace QaplaWindows {

std::vector<std::string> OsDialogs::openFileDialog(bool multiple,
    const std::vector<std::pair<std::string, std::string>>& filters) {
    std::vector<std::string> results;

    // pool = [[NSAutoreleasePool alloc] init];
    Class NSAutoreleasePool = (Class)objc_getClass("NSAutoreleasePool");
    id pool = ((id(*)(Class, SEL))objc_msgSend)(NSAutoreleasePool, sel_registerName("alloc"));
    pool = ((id(*)(id, SEL))objc_msgSend)(pool, sel_registerName("init"));

    Class NSOpenPanel = (Class)objc_getClass("NSOpenPanel");
    id panel = ((id(*)(Class, SEL))objc_msgSend)(NSOpenPanel, sel_registerName("openPanel"));

    ((void(*)(id, SEL, BOOL))objc_msgSend)(
        panel, sel_registerName("setAllowsMultipleSelection:"), multiple ? (BOOL)1 : (BOOL)0);

    // Set file type filters (NSOpenPanel only supports extensions, not descriptions)
    if (!filters.empty()) {
        Class NSMutableArray = (Class)objc_getClass("NSMutableArray");
        id allowedTypes = ((id(*)(Class, SEL))objc_msgSend)(NSMutableArray, sel_registerName("array"));
        
        for (const auto& filter : filters) {
            // Extract file extensions from pattern (e.g., "*.pgn" -> "pgn")
            std::string pattern = filter.second;
            if (pattern.find("*.") == 0 && pattern.length() > 2) {
                std::string ext = pattern.substr(2);
                Class NSString = (Class)objc_getClass("NSString");
                id extStr = ((id(*)(Class, SEL, const char*))objc_msgSend)(
                    NSString, sel_registerName("stringWithUTF8String:"), ext.c_str());
                ((void(*)(id, SEL, id))objc_msgSend)(allowedTypes, sel_registerName("addObject:"), extStr);
            }
        }
        
        if (((NSUInteger(*)(id, SEL))objc_msgSend)(allowedTypes, sel_registerName("count")) > 0) {
            ((void(*)(id, SEL, id))objc_msgSend)(panel, sel_registerName("setAllowedFileTypes:"), allowedTypes);
        }
    }

    // NSModalResponseOK == 1
    NSInteger r = ((NSInteger(*)(id, SEL))objc_msgSend)(panel, sel_registerName("runModal"));
    if (r == 1) {
        id urls = ((id(*)(id, SEL))objc_msgSend)(panel, sel_registerName("URLs"));
        NSUInteger count = ((NSUInteger(*)(id, SEL))objc_msgSend)(urls, sel_registerName("count"));
        for (NSUInteger i = 0; i < count; ++i) {
            id url  = ((id(*)(id, SEL, NSUInteger))objc_msgSend)(urls, sel_registerName("objectAtIndex:"), i);
            id path = ((id(*)(id, SEL))objc_msgSend)(url, sel_registerName("path"));
            const char* cstr = ((const char*(*)(id, SEL))objc_msgSend)(path, sel_registerName("UTF8String"));
            if (cstr) results.emplace_back(cstr);
        }
    }

    // [pool drain];  (alternativ: release)
    ((void(*)(id, SEL))objc_msgSend)(pool, sel_registerName("drain"));
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


// ============================================================================
// FOLDER SELECTION DIALOG
// ============================================================================

std::string OsDialogs::selectFolderDialog(const std::string& defaultPath) {
    std::string result;

    Class poolClass = (Class)objc_getClass("NSAutoreleasePool");
    id pool = ((id(*)(Class, SEL))objc_msgSend)(poolClass, sel_registerName("alloc"));
    pool = ((id(*)(id, SEL))objc_msgSend)(pool, sel_registerName("init"));

    // Create NSOpenPanel
    Class panelClass = (Class)objc_getClass("NSOpenPanel");
    id panel = ((id(*)(Class, SEL))objc_msgSend)(panelClass, sel_registerName("openPanel"));

    // Configure for folder selection
    ((void(*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setCanChooseFiles:"), NO);
    ((void(*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setCanChooseDirectories:"), YES);
    ((void(*)(id, SEL, BOOL))objc_msgSend)(panel, sel_registerName("setAllowsMultipleSelection:"), NO);

    // Set default path if provided
    if (!defaultPath.empty()) {
        id url = ((id(*)(Class, SEL, const char*))objc_msgSend)((Class)objc_getClass("NSURL"), sel_registerName("fileURLWithPath:"), defaultPath.c_str());
        ((void(*)(id, SEL, id))objc_msgSend)(panel, sel_registerName("setDirectoryURL:"), url);
    }

    // Run modal
    NSInteger r = ((NSInteger(*)(id, SEL))objc_msgSend)(panel, sel_registerName("runModal"));
    if (r == 1) {
        id url = ((id(*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
        id path = ((id(*)(id, SEL))objc_msgSend)(url, sel_registerName("path"));
        const char* cstr = ((const char*(*)(id, SEL))objc_msgSend)(path, sel_registerName("UTF8String"));
        if (cstr) result = cstr;
    }

    ((void(*)(id, SEL))objc_msgSend)(pool, sel_registerName("drain"));
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
// For Windows and Linux: Simply call the synchronous version and pass result to callback.
// For macOS: Could be implemented with native async sheets in the future.

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

#endif
