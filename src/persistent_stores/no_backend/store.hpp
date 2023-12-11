#pragma once
/**
 * @brief 'No backend' storing strategy means that store items are only a simple set/get with a default value. No persistent saving at all. This is useful for when an existing store with a persistent strategy is desired to be used as-is in another project, but without the persistent capabilities (eg expecting persistency to be handled by another board in the project)
 *
 */

#include "store_item.hpp"
#include <cstdint>

namespace journal {
// mock to allow easier change of journal-based store into no_backend-based store
consteval uint16_t hash(const char *) {
    return 0;
}
} // namespace journal

namespace no_backend {

struct CurrentStoreConfig {
    template <typename DataT, const DataT &default_val>
    using StoreItem = CurrentItem<DataT, default_val>;
};

struct DeprecatedStoreConfig {
    template <typename DataT, const DataT &DefaultVal>
    using StoreItem = DeprecatedItem<DataT, DefaultVal>;
};

// shorthand to allow easier change of journal-based store into no_backend-based store
struct NBJournalCurrentStoreConfig {
    template <typename DataT, const DataT &default_val, uint16_t hash>
    using StoreItem = CurrentItem<DataT, default_val>;
};

// shorthand to allow easier change of journal-based store into no_backend-based store
struct NBJournalDeprecatedStoreConfig {
    template <typename DataT, const DataT &DefaultVal, uint16_t hash>
    using StoreItem = DeprecatedItem<DataT, DefaultVal>;
};

/**
 * This class takes Config class as template parameter and it defines the items in ConfigStore, DeprecatedItems class has definition for deprecated items.
 */
template <class Config, class DeprecatedItems>
class Store : public Config {
public:
    Store() = default;
    Store(const Store &other) = delete;
    Store(Store &&other) = delete;
    Store &operator=(const Store &other) = delete;
    Store &operator=(Store &&other) = delete;
};

template <class Config, class DeprecatedItems>
inline Store<Config, DeprecatedItems> &store() {
    static Store<Config, DeprecatedItems> str {};
    return str;
}
} // namespace no_backend
