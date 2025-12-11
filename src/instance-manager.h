#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace QaplaWindows {

/**
 * @brief Generic instance manager that tracks objects by key and
 *        returns an RAII handle for automatic deregistration.
 */
template <typename Key, typename T>
class InstanceManager {
public:
    class UnregisterHandle {
    public:
        UnregisterHandle() = default;

        UnregisterHandle(InstanceManager* manager, const Key& key)
            : manager_(manager)
            , key_(key) {}

        ~UnregisterHandle() {
            unregister();
        }

        UnregisterHandle(const UnregisterHandle&) = delete;
        UnregisterHandle& operator=(const UnregisterHandle&) = delete;

        UnregisterHandle(UnregisterHandle&& other) noexcept
            : manager_(other.manager_)
            , key_(std::move(other.key_)) {
            other.manager_ = nullptr;
        }

        UnregisterHandle& operator=(UnregisterHandle&& other) noexcept {
            if (this != &other) {
                unregister();
                manager_ = other.manager_;
                key_ = std::move(other.key_);
                other.manager_ = nullptr;
            }
            return *this;
        }

    private:
        void unregister() {
            if (manager_ != nullptr) {
                manager_->unregisterInstance(key_);
                manager_ = nullptr;
            }
        }

        InstanceManager* manager_ = nullptr;
        Key key_{};
    };

    using HandlePtr = std::unique_ptr<UnregisterHandle>;

    /**
     * @brief Registers an instance under the given key.
     * @return RAII handle that will deregister the instance on destruction.
     */
    HandlePtr registerInstance(const Key& key, T* instance) {
        {
            std::scoped_lock lock(mutex_);
            instances_[key] = instance;
        }
        return std::make_unique<UnregisterHandle>(this, key);
    }

    /**
     * @brief Explicitly unregisters an instance.
     */
    void unregisterInstance(const Key& key) {
        std::scoped_lock lock(mutex_);
        instances_.erase(key);
    }

    /**
     * @brief Returns the instance for a key or nullptr.
     */
    T* get(const Key& key) const {
        std::scoped_lock lock(mutex_);
        auto it = instances_.find(key);
        if (it == instances_.end()) {
            return nullptr;
        }
        return it->second;
    }

    /**
     * @brief Returns a snapshot of all registered keys.
     */
    std::vector<Key> getKeys() const {
        std::scoped_lock lock(mutex_);
        std::vector<Key> result;
        result.reserve(instances_.size());
        for (const auto& [key, value] : instances_) {
            static_cast<void>(value);
            result.push_back(key);
        }
        return result;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<Key, T*> instances_;
};

} // namespace QaplaWindows
