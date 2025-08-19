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

#pragma once

#include "game-record.h"

#include <mutex>
#include <memory>

namespace QaplaTester {

	struct EngineExchangeData {
		std::string name;
		std::string identifier;
		std::string status;
		size_t memoryUsageB;
		bool white;
	};
	using EngineExchangeDataList = std::vector<EngineExchangeData>;
	using ProviderId_t = uint32_t;

	enum ProviderType {
		None,
		ComputeTask,
		GameManager
	};

	template <typename T>
	class Tracked {
	public:
		Tracked(ProviderId_t id) 
			: data_{}, changeCounter_(0), uniqueId_(id) {}
		Tracked(const Tracked& other) = default;
		explicit Tracked(const T& value, ProviderId_t id)
			: data_(value), changeCounter_(1), uniqueId_(id) {}

		/**
		 * @brief Sets the value of the tracked data and increments the change counter.
		 * @param value The new value to set.
		 */
		void set(T value) {
			data_ = std::move(value);
			++changeCounter_;
		}

		/**
		 * @brief Gets the current value of the tracked data.
		 * @return The current value.
		 */
		const T& value() const {
			return data_;
		}

		/**
		 * @brief Gets the current change counter value.
		 * @return The change counter.
		 */
		uint32_t changeCounter() const {
			return changeCounter_;
		}

		ProviderId_t id() const {
			return uniqueId_;
		}

	private:
		T data_;
		uint32_t changeCounter_ = 0; ///< Counter to track changes
		ProviderId_t uniqueId_;
	};

	struct ProviderData {
		ProviderId_t uniqueId;
		ProviderType type;
		Tracked<EngineExchangeDataList> engineDataList;
	};

	class BoardExchange {
	public:

		/** 
		 * @brief Returns the singleton instance of BoardExchange.
		 * @return Reference to the singleton instance.
		 */
		static BoardExchange& instance() {
			static BoardExchange instance;
			return instance;
		}

		/**
		 * @brief Unregisters a provider and removes all associated data.
		 *
		 * This method ensures that all data linked to the specified provider
		 * is cleaned up. It is thread-safe and will grow as additional data
		 * management is implemented.
		 *
		 * @param providerId The unique identifier of the provider to unregister.
		 */
		void unregisterProvider(ProviderId_t id);

		/**
		 * @brief Registers a provider and assigns a unique ID.
		 *
		 * This method registers a new provider of the specified type and assigns
		 * a unique ID to it. The ID can be used to reference the provider in future
		 * operations.
		 *
		 * @param type The type of the provider (e.g., ComputeTask, GameManager).
		 * @return The unique ID assigned to the provider.
		 */
		ProviderId_t registerProvider(ProviderType type);

		/**
		 * @brief Sets a new EngineExchangeDataList for a specific provider.
		 *
		 * This method allows a provider to update its associated engine data list.
		 * The function is thread-safe and ensures the data is stored correctly.
		 *
		 * @param id The unique identifier of the provider.
		 * @param engineDataList The new engine data list to associate with the provider.
		 */
		void setEngineDataList(ProviderId_t id, const EngineExchangeDataList& engineDataList);

		/**
		 * @brief Retrieves all IDs associated with a specific ProviderType.
		 *
		 * This method iterates through all active providers and collects the IDs
		 * of those matching the specified ProviderType.
		 *
		 * @param type The ProviderType to filter by (e.g., ComputeTask, GameManager).
		 * @return A vector of ProviderId_t containing all matching IDs.
		 */
		std::vector<ProviderId_t> getIdsByProviderType(ProviderType type) const;

		/**
		 * @brief Retrieves a vector of Tracked<EngineExchangeDataList> for a given list of IDs.
		 *
		 * This method ensures that the returned vector contains the same number of elements
		 * as the input list of IDs, in the same order. If an ID is not found, a default-constructed
		 * Tracked<EngineExchangeDataList> is added in its place.
		 *
		 * @param ids A vector of ProviderId_t representing the IDs to retrieve.
		 * @return A vector of Tracked<EngineExchangeDataList> corresponding to the given IDs.
		 */
		std::vector<Tracked<EngineExchangeDataList>> getTrackedEngineDataLists(
			const std::vector<ProviderId_t>& ids) const;

	private:

		ProviderId_t nextId_ = 0; ///< Unique ID for the next provider
		mutable std::mutex exchangeMutex_; ///< Mutex for thread-safe access to the exchange data
		std::unordered_map<ProviderId_t, ProviderData> activeProviders_; ///< Map of active registrations
	};

}
