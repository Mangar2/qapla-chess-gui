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
 * WINDOWS IMPLEMENTATION
 * =============================================================================
 * 
 * Uses glfwGetWin32Window() to get native HWND from current GLFW context
 * Passes HWND to dialog functions (IFileOpenDialog::Show, OPENFILENAME::hwndOwner)
 * Dialogs are properly parented and stay on top of main window
 * 
 * =============================================================================
 */

#ifdef _WIN32

#include "os-dialogs.h"
#include "base-elements/string-helper.h"
#include <vector>
#include <filesystem>
#include <string>
#include <ranges>

// GLFW for getting native window handle
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <shobjidl.h> 
#include <comdef.h>
#include <shlobj.h>

namespace {
    using namespace QaplaHelpers;

    /**
     * Gets the native Win32 window handle from the current GLFW context.
     * @return HWND of the current window, or nullptr if no context.
     */
    HWND getNativeWindowHandle() {
        GLFWwindow* window = glfwGetCurrentContext();
        if (window == nullptr) {
            return nullptr;
        }
        return glfwGetWin32Window(window);
    }

    void setDialogOptions(IFileOpenDialog* pFileOpen, bool multiple) {
        DWORD dwOptions;
        if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
            dwOptions |= FOS_FORCEFILESYSTEM;
            if (multiple) {
                dwOptions |= FOS_ALLOWMULTISELECT;
            }
            pFileOpen->SetOptions(dwOptions);
        }
    }

    void setFileFilters(IFileOpenDialog* pFileOpen,
                        const std::vector<std::pair<std::string, std::string>>& filters) {
        if (filters.empty()) return;

        std::vector<std::wstring> nameStrings;
        std::vector<std::wstring> specStrings;
        std::vector<COMDLG_FILTERSPEC> fileTypes;

        nameStrings.reserve(filters.size());
        specStrings.reserve(filters.size());
        fileTypes.reserve(filters.size());

        for (const auto& [desc, pattern] : filters) {
            nameStrings.push_back(ascii_to_wstring(desc));
            specStrings.push_back(ascii_to_wstring(pattern));
            // COMDLG_FILTERSPEC stores only pointers, not strings. nameStrings/specStrings keep the actual data alive.
            fileTypes.push_back({nameStrings.back().c_str(), specStrings.back().c_str()});
        }

        pFileOpen->SetFileTypes(static_cast<UINT>(fileTypes.size()), fileTypes.data());
        pFileOpen->SetFileTypeIndex(1);
    }

    std::string wideCharToString(PWSTR wstr) {
        char buffer[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buffer, MAX_PATH, nullptr, nullptr);
        return buffer;
    }

    void collectMultipleResults(IShellItemArray* pResults, std::vector<std::string>& results) {
        DWORD count = 0;
        pResults->GetCount(&count);
        for (DWORD i = 0; i < count; i++) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pResults->GetItemAt(i, &pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    results.push_back(wideCharToString(pszFilePath));
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
    }

    void collectSingleResult(IFileOpenDialog* pFileOpen, std::vector<std::string>& results) {
        IShellItem* pItem = nullptr;
        if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
            PWSTR pszFilePath = nullptr;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                results.push_back(wideCharToString(pszFilePath));
                CoTaskMemFree(pszFilePath);
            }
            pItem->Release();
        }
    }
}

namespace QaplaWindows {
std::vector<std::string> OsDialogs::openFileDialog(bool multiple,
    const std::vector<std::pair<std::string, std::string>>& filters) {
    std::vector<std::string> results;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return results;

    IFileOpenDialog* pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) {
        setDialogOptions(pFileOpen, multiple);
        setFileFilters(pFileOpen, filters);

        hr = pFileOpen->Show(getNativeWindowHandle());
        if (SUCCEEDED(hr)) {
            if (multiple) {
                IShellItemArray* pResults = nullptr;
                if (SUCCEEDED(pFileOpen->GetResults(&pResults))) {
                    collectMultipleResults(pResults, results);
                    pResults->Release();
                }
            } else {
                collectSingleResult(pFileOpen, results);
            }
        }
        pFileOpen->Release();
    }

    CoUninitialize();
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

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = getNativeWindowHandle();
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

// ============================================================================
// FOLDER SELECTION DIALOG
// ============================================================================

std::string OsDialogs::selectFolderDialog(const std::string& defaultPath) {
    std::string result;

    // Initialize COM (thread-safe)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return result;
    }

    IFileOpenDialog* pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr)) {
        // Options for folder selection
        DWORD dwOptions;
        if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
            dwOptions |= FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM;
            pFileOpen->SetOptions(dwOptions);
        }

        // Set default path if provided
        if (!defaultPath.empty()) {
            IShellItem* pItem = nullptr;
            wchar_t wDefaultPath[MAX_PATH];
            MultiByteToWideChar(CP_UTF8, 0, defaultPath.c_str(), -1, wDefaultPath, MAX_PATH);
            
            hr = SHCreateItemFromParsingName(wDefaultPath, nullptr, IID_IShellItem, 
                                             reinterpret_cast<void**>(&pItem));
            if (SUCCEEDED(hr)) {
                pFileOpen->SetDefaultFolder(pItem);
                pItem->Release();
            }
        }

        // Show dialog
        hr = pFileOpen->Show(getNativeWindowHandle());
        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFolderPath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath))) {
                    char buffer[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, pszFolderPath, -1, buffer, MAX_PATH, nullptr, nullptr);
                    result = buffer;
                    CoTaskMemFree(pszFolderPath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }

    CoUninitialize();
    return result;
}

std::string OsDialogs::getConfigDirectory() {
    // Try using Windows API first (most robust)
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        std::filesystem::path configPath(path);
        configPath /= "qapla-chess-gui";
        return configPath.string();
    }
    
    // Fallback to environment variable
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "LOCALAPPDATA") == 0 && buf != nullptr) {
        std::string localAppData(buf);
        free(buf);
        return localAppData + "/qapla-chess-gui";
    }
    
    // Last fallback for Windows
    return "./qapla-chess-gui";
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

#endif // _WIN32


