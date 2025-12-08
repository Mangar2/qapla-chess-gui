/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

#include "os-helpers.h"

#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <format>
#include <array>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <sysinfoapi.h>
#include <intrin.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

namespace QaplaHelpers {

std::string OsHelpers::getConfigDirectory() {
    namespace fs = std::filesystem;

#ifdef _WIN32
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, "LOCALAPPDATA") == 0 && buf != nullptr) {
        std::string path(buf);
        free(buf);
        return path + "/qapla-chess-gui";
    }
    // Fallback if LOCALAPPDATA is not set
    return std::string(".") + "/qapla-chess-gui";
#else
    return std::string(std::getenv("HOME")) + "/.qapla-chess-gui";
#endif
}

void OsHelpers::openInShell(const std::string& path) {
#ifdef _WIN32
    // Windows: Use ShellExecuteA to open with default application
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
    // macOS: Use "open" command
    std::string command = "open \"" + path + "\"";
    std::system(command.c_str());
#else
    // Linux: Use xdg-open
    std::string command = "xdg-open \"" + path + "\"";
    std::system(command.c_str());
#endif
}

std::string OsHelpers::getHostname() {
#ifdef _WIN32
    char buffer[256];
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size) != 0) {
        return std::string(buffer);
    }
    return "Unknown";
#else
    char buffer[256];
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        return std::string(buffer);
    }
    return "Unknown";
#endif
}

std::string OsHelpers::getOperatingSystem() {
#ifdef _WIN32
    std::ostringstream oss;
    
    // Get Windows version using RtlGetVersion
    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll != nullptr) {
        auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (RtlGetVersion != nullptr) {
            RTL_OSVERSIONINFOW version = {};
            version.dwOSVersionInfoSize = sizeof(version);
            if (RtlGetVersion(&version) == 0) {
                oss << "Windows ";
                
                // Determine Windows version
                if (version.dwMajorVersion == 10 && version.dwBuildNumber >= 22000) {
                    oss << "11";
                } else if (version.dwMajorVersion == 10) {
                    oss << "10";
                } else {
                    oss << version.dwMajorVersion << "." << version.dwMinorVersion;
                }
                
                oss << " (Build " << version.dwBuildNumber << ")";
                
                // Get architecture
                SYSTEM_INFO sysInfo;
                GetNativeSystemInfo(&sysInfo);
                if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                    oss << " 64 bit";
                } else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
                    oss << " 32 bit";
                } else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
                    oss << " ARM64";
                }
                
                return oss.str();
            }
        }
    }
    return "Windows";
#elif defined(__APPLE__)
    std::ostringstream oss;
    char version[256];
    size_t len = sizeof(version);
    if (sysctlbyname("kern.osproductversion", version, &len, nullptr, 0) == 0) {
        oss << "macOS " << version;
    } else {
        oss << "macOS";
    }
    return oss.str();
#else
    // Linux: Try to get distribution info
    std::ostringstream oss;
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        oss << unameData.sysname << " " << unameData.release;
        
        // Try to get distribution name from /etc/os-release
        std::ifstream osRelease("/etc/os-release");
        if (osRelease.is_open()) {
            std::string line;
            while (std::getline(osRelease, line)) {
                if (line.find("PRETTY_NAME=") == 0) {
                    auto start = line.find('"');
                    auto end = line.rfind('"');
                    if (start != std::string::npos && end != std::string::npos && end > start) {
                        return line.substr(start + 1, end - start - 1);
                    }
                }
            }
        }
    }
    return oss.str().empty() ? "Linux" : oss.str();
#endif
}

std::string OsHelpers::getHardwareInfo() {
    std::ostringstream oss;
    
#ifdef _WIN32
    // Get CPU name from registry
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char cpuName[256];
        DWORD bufSize = sizeof(cpuName);
        if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(cpuName), &bufSize) == ERROR_SUCCESS) {
            // Trim leading/trailing spaces
            std::string cpu(cpuName);
            size_t start = cpu.find_first_not_of(" \t");
            size_t end = cpu.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                oss << cpu.substr(start, end - start + 1);
            }
        }
        RegCloseKey(hKey);
    }
    
    // Get memory
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus) != 0) {
        double gbMemory = static_cast<double>(memStatus.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
        oss << std::format(" mit {:.1f} GB Speicher", gbMemory);
    }
    
#elif defined(__APPLE__)
    // Get CPU brand string
    char cpuBrand[256];
    size_t len = sizeof(cpuBrand);
    if (sysctlbyname("machdep.cpu.brand_string", cpuBrand, &len, nullptr, 0) == 0) {
        oss << cpuBrand;
    }
    
    // Get memory
    int64_t memSize = 0;
    len = sizeof(memSize);
    if (sysctlbyname("hw.memsize", &memSize, &len, nullptr, 0) == 0) {
        double gbMemory = static_cast<double>(memSize) / (1024.0 * 1024.0 * 1024.0);
        oss << std::format(" with {:.1f} GB Memory", gbMemory);
    }
    
#else
    // Linux: Read CPU info from /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") == 0) {
                auto pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string cpu = line.substr(pos + 1);
                    // Trim
                    size_t start = cpu.find_first_not_of(" \t");
                    if (start != std::string::npos) {
                        oss << cpu.substr(start);
                    }
                    break;
                }
            }
        }
    }
    
#ifdef __linux__
    // Get memory from sysinfo (Linux only)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        double gbMemory = static_cast<double>(info.totalram) * info.mem_unit / (1024.0 * 1024.0 * 1024.0);
        oss << std::format(" with {:.1f} GB Memory", gbMemory);
    }
#endif
#endif
    
    return oss.str().empty() ? "Unknown" : oss.str();
}

std::string OsHelpers::getCountry() {
#ifdef _WIN32
    // Get locale information from Windows
    GEOID geoId = GetUserGeoID(GEOCLASS_NATION);
    if (geoId != GEOID_NOT_AVAILABLE) {
        char countryName[256];
        int result = GetGeoInfoA(geoId, GEO_FRIENDLYNAME, countryName, sizeof(countryName), 0);
        if (result != 0) {
            return std::string(countryName);
        }
    }
    return "Unknown";
#elif defined(__APPLE__)
    // Try to get locale from environment
    const char* locale = std::getenv("LANG");
    if (locale != nullptr) {
        std::string loc(locale);
        // Extract country code (e.g., "en_US.UTF-8" -> "US")
        auto underscorePos = loc.find('_');
        auto dotPos = loc.find('.');
        if (underscorePos != std::string::npos && dotPos != std::string::npos) {
            return loc.substr(underscorePos + 1, dotPos - underscorePos - 1);
        }
    }
    return "Unknown";
#else
    // Linux: Try to get from locale
    const char* locale = std::getenv("LANG");
    if (locale != nullptr) {
        std::string loc(locale);
        // Extract country code
        auto underscorePos = loc.find('_');
        auto dotPos = loc.find('.');
        if (underscorePos != std::string::npos && dotPos != std::string::npos) {
            return loc.substr(underscorePos + 1, dotPos - underscorePos - 1);
        }
    }
    return "Unknown";
#endif
}

} // namespace QaplaHelpers
