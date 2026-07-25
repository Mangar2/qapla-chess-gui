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

#include "lm-studio-locator.h"

#include <httplib.h>

#include <cstdlib>
#include <system_error>

namespace QaplaLlm {

bool LmStudioLocator::probeServer(const LmStudioProbeConfig& config) {
    httplib::Client client(config.host, config.port);
    const time_t sec = config.timeoutMs / 1000;
    const time_t usec = (config.timeoutMs % 1000) * 1000;
    client.set_connection_timeout(sec, usec);
    client.set_read_timeout(sec, usec);
    client.set_write_timeout(sec, usec);

    auto res = client.Get("/v1/models");
    return res && res->status == 200;
}

std::vector<std::filesystem::path> LmStudioLocator::defaultInstallPaths() {
    std::vector<std::filesystem::path> paths;

#if defined(_WIN32)
    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        paths.push_back(std::filesystem::path(localAppData) / "Programs" / "LM Studio");
    }
    if (const char* userProfile = std::getenv("USERPROFILE")) {
        paths.push_back(std::filesystem::path(userProfile) / ".lmstudio");
    }
#elif defined(__APPLE__)
    paths.emplace_back("/Applications/LM Studio.app");
    if (const char* home = std::getenv("HOME")) {
        paths.push_back(std::filesystem::path(home) / ".lmstudio");
    }
#else
    if (const char* home = std::getenv("HOME")) {
        paths.push_back(std::filesystem::path(home) / ".lmstudio");
    }
#endif

    return paths;
}

bool LmStudioLocator::isInstalledAt(const std::vector<std::filesystem::path>& candidatePaths) {
    for (const auto& path : candidatePaths) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            return true;
        }
    }
    return false;
}

bool LmStudioLocator::isInstalled() {
    return isInstalledAt(defaultInstallPaths());
}

LmStudioStatus LmStudioLocator::detect(const LmStudioProbeConfig& config) {
    if (probeServer(config)) {
        return LmStudioStatus::ServerRunning;
    }
    if (isInstalled()) {
        return LmStudioStatus::InstalledServerDown;
    }
    return LmStudioStatus::NotInstalled;
}

AsyncLmStudioLocator::AsyncLmStudioLocator(LmStudioProbeConfig config)
    : config_(std::move(config)) {
}

void AsyncLmStudioLocator::start() {
    future_ = std::async(std::launch::async, [config = config_]() {
        return LmStudioLocator::detect(config);
    });
}

bool AsyncLmStudioLocator::isReady() {
    if (ready_) {
        return true;
    }
    if (!future_.valid()) {
        return false;
    }
    if (future_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return false;
    }
    status_ = future_.get();
    ready_ = true;
    return true;
}

} // namespace QaplaLlm
