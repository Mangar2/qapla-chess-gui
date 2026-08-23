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

#pragma once

#include <optional>
#include <string>

namespace QaplaHelpers {

/**
 * @brief Operating system helper functions.
 * 
 * Provides cross-platform utilities for OS-specific operations like
 * getting standard directories and opening files in the default application.
 */
class OsHelpers {
public:
    OsHelpers() = delete;
    ~OsHelpers() = delete;

    /**
     * @brief Reads an environment variable.
     *
     * Use this instead of std::getenv: the MSVC runtime marks that function deprecated in favour
     * of _dupenv_s, which hands out an owned buffer that the caller has to free again.
     *
     * @param name Name of the variable.
     * @return Its value, or std::nullopt if the variable is not set.
     */
    static std::optional<std::string> getEnv(const std::string& name);

    /**
     * @brief Puts everything this session stores into @p directory instead of the user's own
     *        configuration directory.
     *
     * This is the one switch that makes an automated test run harmless. Everything the GUI keeps
     * between sessions is addressed through getConfigDirectory(): qapla-chess-gui.ini with the
     * engine list and every window's settings, the EPD results, the chat and finetuning logs, the
     * auto-saved PGN. Redirecting that one function is therefore enough to leave the
     * configuration the user works with untouched -- and, just as important, to keep a test from
     * silently depending on what an earlier session happened to leave behind.
     *
     * Set once at startup from --config-dir, before the first setting is read; see
     * QaplaApp::CommandLineOptions::configDirectory.
     *
     * @param directory Where to keep this session's files. Empty restores the per-user default.
     */
    static void setConfigDirectoryOverride(std::string directory);

    /**
     * @brief What setConfigDirectoryOverride() was given, or an empty string when it was not used.
     *
     * For the few places that need to know whether this session is running out of a directory of
     * its own, or that compute a configuration path themselves instead of calling
     * getConfigDirectory() -- today the Windows and Linux variants of OsDialogs.
     */
    static std::string configDirectoryOverride();

    /**
     * @brief Gets the configuration directory for the application.
     *
     * A directory set with setConfigDirectoryOverride() wins. Without one, the platform-specific
     * per-user directory:
     * - Windows: %LOCALAPPDATA%/qapla-chess-gui
     * - Linux/macOS: ~/.qapla-chess-gui
     *
     * @return Path to the configuration directory. Falls back to a relative path below the
     *         working directory when the home directory cannot be determined.
     */
    static std::string getConfigDirectory();

    /**
     * @brief Opens a file or URL in the default system application.
     * 
     * Uses platform-specific methods:
     * - Windows: ShellExecuteA with "open" verb
     * - macOS: "open" command
     * - Linux: "xdg-open" command
     * 
     * @param path File path or URL to open.
     */
    static void openInShell(const std::string& path);

    /**
     * @brief Gets the hostname of the system.
     * 
     * @return Hostname as string, or "Unknown" if unavailable.
     */
    static std::string getHostname();

    /**
     * @brief Gets the operating system name and version.
     * 
     * Returns platform-specific OS information:
     * - Windows: "Windows <version> <edition> (Build <build>) <arch>"
     * - Linux: Distribution and kernel version
     * - macOS: "macOS <version>"
     * 
     * @return OS description string.
     */
    static std::string getOperatingSystem();

    /**
     * @brief Gets hardware information (CPU model and memory).
     * 
     * Returns CPU model and total system memory:
     * - Example: "Intel(R) Core(TM) i5-11400H @ 2.70GHz mit 15,6 GB Speicher"
     * 
     * @return Hardware description string.
     */
    static std::string getHardwareInfo();

    /**
     * @brief Gets the country/location based on system locale.
     * 
     * @return Country name or "Unknown" if unavailable.
     */
    static std::string getCountry();

private:
    /**
     * @brief The per-user configuration directory of this operating system, ignoring any override.
     *
     * The macOS variant is implemented in os-helpers-apple.cpp, all others in os-helpers.cpp, so a
     * change to one cannot reach the other. getConfigDirectory() is the only caller.
     */
    static std::string defaultConfigDirectory();
};

} // namespace QaplaHelpers
