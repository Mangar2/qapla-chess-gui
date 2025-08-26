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

#include "board-exchange.h"

using namespace QaplaTester;

void BoardExchange::unregisterProvider(ProviderId_t id) {
    std::lock_guard<std::mutex> lock(exchangeMutex_);
    activeProviders_.erase(id);
}

uint32_t BoardExchange::registerProvider() {
    std::lock_guard<std::mutex> lock(exchangeMutex_);
    uint32_t newId = nextId_++;
    activeProviders_.emplace(newId, ProviderData{
        .uniqueId = newId,
        .engineDataList = Tracked<EngineExchangeDataList>(newId),
        .gameRecord = Tracked<GameRecord>(newId)
        });
    return newId;
}

ProviderData* BoardExchange::getProviderData(ProviderId_t id) {
    auto it = activeProviders_.find(id);
    if (it != activeProviders_.end()) {
        return &it->second;
    }
    return nullptr;
}

void BoardExchange::setEngineDataList(ProviderId_t id, const EngineExchangeDataList& engineDataList) {
    std::lock_guard<std::mutex> lock(exchangeMutex_);
    auto providerData = getProviderData(id);
    if (providerData) {
        providerData->engineDataList = Tracked<EngineExchangeDataList>(engineDataList, id);
    } 
    else {
        throw std::runtime_error("Provider ID not found in active providers.");
    }
}

std::vector<Tracked<EngineExchangeDataList>> BoardExchange::getTrackedEngineDataLists(
    const std::vector<ProviderId_t>& ids) const
{
    std::lock_guard<std::mutex> lock(exchangeMutex_);
    std::vector<Tracked<EngineExchangeDataList>> result;
    result.reserve(ids.size());

    for (const auto& id : ids) {
        auto it = activeProviders_.find(id);
        if (it != activeProviders_.end()) {
            result.push_back(it->second.engineDataList);
        }
    }

    return result;
}

void BoardExchange::setGameRecord(const GameRecord& record, ProviderId_t id) {
    std::lock_guard<std::mutex> lock(exchangeMutex_);
    auto it = activeProviders_.find(id);
    if (it != activeProviders_.end()) {
        it->second.gameRecord.set(record.createMinimalCopy());
    }
    else {
        throw std::runtime_error("Provider ID not found in active providers.");
    }
}

void BoardExchange::modifyGameRecordThreadSafe(ProviderId_t id, const std::function<void(GameRecord&)>& callback) {
    std::lock_guard<std::mutex> lock(exchangeMutex_);
    auto it = activeProviders_.find(id);
    if (it != activeProviders_.end()) {
        callback(it->second.gameRecord.value());
    }
    else {
        throw std::runtime_error("Provider ID not found in active providers.");
    }
}

void BoardExchange::setComputationState(ProviderId_t id, ComputationStatus status) {
    std::lock_guard<std::mutex> lock(exchangeMutex_);
    auto* providerData = getProviderData(id);
    if (providerData) {
        providerData->status = status;
    }
    else {
        throw std::runtime_error("Provider ID not found in active providers.");
    }
}