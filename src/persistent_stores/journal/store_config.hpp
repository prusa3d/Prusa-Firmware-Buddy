/// @file
#pragma once

#include <cstring>
#include "store_item.hpp"
#include "store_item_array.hpp"
#include <algorithm>
#include <ranges>
#include <type_traits>

#include "utils/utility_extensions.hpp"
#include "backend.hpp"
#include <persistent_stores/journal/gen_journal_hashes.hpp>

namespace journal {

consteval uint16_t hash(std::string_view name) {
    return get_generated_hash(name);
}

template <BackendC BackendT, auto backend>
struct CurrentStoreConfig {
    static inline BackendT &get_backend() { return backend(); };
    using Backend = BackendT;

    template <StoreItemDataC DataT, auto default_val, ItemFlags flags, typename BackendT::Id id, uint8_t hash_alloc_range = 1, bool ram_only = false>
    using StoreItem = JournalItem<DataT, default_val, flags, backend, id, hash_alloc_range, ram_only>;

    template <StoreItemDataC DataT, auto default_val, ItemFlags flags, typename BackendT::Id id, uint8_t max_item_count, uint8_t item_count>
    using StoreItemArray = JournalItemArray<DataT, default_val, flags, backend, id, max_item_count, item_count>;

    template <StoreItemDataC DataT, auto default_val, ItemFlags flags, auto hashed_ids>
    using StoreItemLegacyArray = JournalItemLegacyArray<DataT, default_val, flags, backend, hashed_ids>;
};

template <BackendC BackendT>
struct DeprecatedStoreConfig {
    // we don't care about default val, but we have it anyway to make deprecating an item a ctrl+c and ctrl+v operation (and in case we need it for some reason)
    template <StoreItemDataC DataT, auto DefaultVal, typename BackendT::Id HashedID>
    using StoreItem
        = DeprecatedStoreItem<DataT, DefaultVal, BackendT, HashedID>;

    template <StoreItemDataC DataT, auto DefaultVal, typename BackendT::Id HashedID, uint8_t max_item_count, uint8_t item_count>
    using StoreItemArray
        = DeprecatedStoreItemArray<DataT, DefaultVal, BackendT, HashedID, max_item_count, item_count>;
};

/**
 * @brief Check whether the store's backend's reserved IDs are not causing a collision with pregenerated hash ids
 *
 * @tparam CurrentStoreT
 */
template <class CurrentStoreT>
bool consteval has_unique_items() {
    for (auto reserved : CurrentStoreT::Backend::RESERVED_IDS) {
        if (auto res = std::ranges::find_if(journal::generated_hashes, [&reserved](const journal::GeneratedPair &elem) {
                return elem.hashed == reserved;
            });
            res != std::end(journal::generated_hashes)) {
            consteval_assert_false("Some newly added Ids cause collision with reserved backend Ids");
            return false;
        }
    }
    return true;
};

} // namespace journal
