#pragma once
#include <cstring>
#include "cthash/sha2/sha256.hpp"
#include "common/to_tie.hpp"
#include "common/extract_member_pointer.hpp"
#include "store_item.hpp"
#include <tuple>
#include <algorithm>
#include <ranges>
#include "indices.hpp"
#include "utils/utility_extensions.hpp"
#include "backend.hpp"

namespace journal {

consteval uint16_t hash(std::string_view name) {
    auto hash = cthash::simple<cthash::sha256>(name);
    auto slice = (static_cast<uint16_t>(hash[0]) << 8) | static_cast<uint16_t>(hash[1]);
    return slice & 0x3FFF;
}

template <BackendC BackendT, BackendT &(*backend)()>
struct CurrentStoreConfig {
    static BackendT &get_backend() { return backend(); };
    using Backend = BackendT;
    template <StoreItemDataC DataT, const DataT &default_val, typename BackendT::Id id>
    using StoreItem = JournalItem<DataT, default_val, BackendT, backend, id>;
};

template <BackendC BackendT>
struct DeprecatedStoreConfig {
    template <StoreItemDataC DataT, const DataT &DefaultVal, typename BackendT::Id HashedID>
    using StoreItem
        = DeprecatedStoreItem<DataT, DefaultVal, BackendT, HashedID>;
};

template <class T, class U>
static auto consteval has_unique_items() {
    using TupleT = typename std::invoke_result<decltype(to_tie<T>), T &>::type;
    using TupleU = typename std::invoke_result<decltype(to_tie<U>), U &>::type;

    using Tuple = decltype(std::tuple_cat(std::declval<TupleT>(), std::declval<TupleU>()));
    constexpr auto index = to_id_array<Tuple>();
    for (size_t i = 0; i < index.size(); i++) {
        for (size_t j = i + 1; j < index.size(); j++) {
            if (index[i] == index[j]) {
                consteval_assert_false("Some newly added Ids cause collision");
            }
        }
        for (auto reserved : T::Backend::RESERVED_IDS) {
            if (index[i] == reserved) {
                consteval_assert_false("Some newly added Ids cause collision with reserved Ids");
            }
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
        std::apply([](auto &&...args) {
            (args.ram_dump(), ...);
        },
            to_tie(*static_cast<Config *>(this)));
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
