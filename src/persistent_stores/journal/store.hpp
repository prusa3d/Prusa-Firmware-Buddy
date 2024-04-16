#pragma once

/**
 * @brief Journal storing strategy means that items are stored persistently using a journal. Changes to item value are stored as an entry to a journal, and upon device reset this journal is read from the beginning, adjusting individual items as the history is being read. Specific implementation may vary (see journal/backend for the one used).
 */

#include <cstring>
#include "common/to_tie.hpp"
#include "common/extract_member_pointer.hpp"
#include "store_item.hpp"
#include <tuple>
#include <algorithm>
#include <ranges>
#include "indices.hpp"
#include "utils/utility_extensions.hpp"
#include "backend.hpp"
#include <persistent_stores/journal/gen_journal_hashes.hpp>

namespace journal {

consteval uint16_t hash(std::string_view name) {
    return get_generated_hash(name);
}

template <BackendC BackendT, BackendT &(*backend)()>
struct CurrentStoreConfig {
    static BackendT &get_backend() { return backend(); };
    using Backend = BackendT;
    template <StoreItemDataC DataT, const DataT &default_val, typename BackendT::Id id>
    using StoreItem = JournalItem<DataT, default_val, backend, id>;
};

template <BackendC BackendT>
struct DeprecatedStoreConfig {
    // we don't care about default val, but we have it anyway to make deprecating an item a ctrl+c and ctrl+v operation (and in case we need it for some reason)
    template <StoreItemDataC DataT, const DataT &DefaultVal, typename BackendT::Id HashedID>
    using StoreItem
        = DeprecatedStoreItem<DataT, DefaultVal, BackendT, HashedID>;
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

/**
 * This class takes Config class as template parameter and it defines the items in ConfigStore, DeprecatedItems class has definition for deprecated items.
 */
template <class Config, class DeprecatedItems, const std::span<const journal::Backend::MigrationFunction> &MigrationFunctions>
class Store : public Config {

    void dump_items() {
        to_tie(*static_cast<Config *>(this)).apply([](auto &&...args) {
            (args.ram_dump(), ...);
        });
    }

public:
    /**
     * @brief Loads data from a byte array with a hashed_id.
     * It's meant to be called only by migrating functions.
     *
     * @param id Hashed id of target store item
     * @param data Holds data in binary form to be loaded into current item
     */
    void load_item(uint16_t id, std::span<uint8_t> data) {
        using TupleT = typename std::invoke_result<decltype(to_tie<Config>), Config &>::type;
        auto constexpr indices = get_current_indices<Config>();
        auto res = std::lower_bound(indices.cbegin(), indices.cend(), detail::CurrentItemIndex<TupleT>(id, nullptr),
            [](const auto &a, const auto &b) { return a.id < b.id; });
        if (res != indices.cend() && res->id == id) { // we found it in current RAM mirror
            // load data to the item itself
            auto tuple = to_tie(*static_cast<Config *>(this));
            res->fnc(data, tuple);
        }
    }

    void save_all() {
        dump_items();
    };

    void load_all() {
        Config::get_backend().load_all([this](uint16_t id, std::span<uint8_t> data) -> void { return load_item(id, data); }, MigrationFunctions);
    };
    void init() {
        Config::get_backend().init([this]() {
            dump_items();
        });
    }

    Store() = default;
    Store(const Store &other) = delete;
    Store(Store &&other) = delete;
    Store &operator=(const Store &other) = delete;
    Store &operator=(Store &&other) = delete;
};

template <class Config, class DeprecatedItems, const std::span<const journal::Backend::MigrationFunction> &MigrationFunctions>
inline Store<Config, DeprecatedItems, MigrationFunctions> &store() {
    static Store<Config, DeprecatedItems, MigrationFunctions> str {};
    return str;
}
} // namespace journal
