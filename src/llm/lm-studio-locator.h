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
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#pragma once

#include <filesystem>
#include <future>
#include <string>
#include <vector>

namespace QaplaLlm {

/**
 * @brief Result of detecting an LM Studio installation on the current machine.
 */
enum class LmStudioStatus {
    NotInstalled,        ///< No installation found and no server reachable.
    InstalledServerDown, ///< Installation found, but the local API server is not running.
    ServerRunning         ///< The OpenAI-compatible local API server answered.
};

/**
 * @brief Connection parameters for probing the LM Studio local server.
 */
struct LmStudioProbeConfig {
    std::string host = "localhost";
    int port = 1234;
    int timeoutMs = 250;
};

/**
 * @brief Stateless detection logic for LM Studio.
 *
 * Kept free of threading/UI concerns so every method can be exercised
 * directly in unit tests (mock HTTP server, injected candidate paths).
 */
class LmStudioLocator {
public:
    /**
     * @brief Probes whether an OpenAI-compatible server answers GET /v1/models.
     * @param config Host/port/timeout to probe.
     * @return true if the server responded with a successful status.
     */
    [[nodiscard]] static bool probeServer(const LmStudioProbeConfig& config);

    /**
     * @brief Platform-specific candidate installation paths.
     *
     * The returned paths are not guaranteed to exist; they are the
     * locations LM Studio is known to install to per platform.
     */
    [[nodiscard]] static std::vector<std::filesystem::path> defaultInstallPaths();

    /**
     * @brief True if any of the given candidate paths exists on disk.
     * @param candidatePaths Paths to check; exposed for testing with temp dirs.
     */
    [[nodiscard]] static bool isInstalledAt(const std::vector<std::filesystem::path>& candidatePaths);

    /**
     * @brief Convenience: isInstalledAt(defaultInstallPaths()).
     */
    [[nodiscard]] static bool isInstalled();

    /**
     * @brief Full detection: server probe first, then installation check.
     * @param config Host/port/timeout to probe.
     */
    [[nodiscard]] static LmStudioStatus detect(const LmStudioProbeConfig& config);
};

/**
 * @brief Runs LmStudioLocator::detect() on a worker thread and exposes the
 * result via polling, so the GUI/render thread never blocks on network or
 * filesystem I/O during startup.
 */
class AsyncLmStudioLocator {
public:
    explicit AsyncLmStudioLocator(LmStudioProbeConfig config = {});

    /**
     * @brief Starts the background probe. Must be called at most once.
     */
    void start();

    /**
     * @brief Non-blocking poll; call every frame until it returns true.
     * @return true once the detection result is available.
     */
    [[nodiscard]] bool isReady();

    /**
     * @brief The detection result. Only valid after isReady() returned true.
     */
    [[nodiscard]] LmStudioStatus status() const {
        return status_;
    }

private:
    LmStudioProbeConfig config_;
    std::future<LmStudioStatus> future_;
    LmStudioStatus status_ = LmStudioStatus::NotInstalled;
    bool ready_ = false;
};

} // namespace QaplaLlm
