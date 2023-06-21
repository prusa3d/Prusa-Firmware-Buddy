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

namespace Journal {

consteval uint16_t hash(std::string_view name) {
    auto hash = cthash::simple<cthash::sha256>(name);
    auto slice = (static_cast<uint16_t>(hash[0]) << 8) | static_cast<uint16_t>(hash[1]);
    return slice & 0x3FFF;
}

template <BackendC BackendT, BackendT &(*backend)()>
struct CurrentStoreConfig {
    static BackendT &get_backend() { return backend(); };
    using Backend = BackendT;
    template <std::default_initializable DataT, const DataT &default_val, uint16_t id>
    using StoreItem = JournalItem<DataT, default_val, BackendT, backend, id>;
};

template <BackendC BackendT>
struct DeprecatedStoreConfig {
    template <typename DataT, uint32_t HashedID, auto ptr>
    using DeprecatedStoreItem
        = DeprecatedJournalStoreItemImpl<DataT, HashedID, BackendT,
            remove_member_pointer_t<decltype(ptr)>, extract_class_type_t<decltype(ptr)>, ptr>;

    template <typename DataT, uint32_t HashedID>
    using DeletedStoreItem = DeletedStoreItemImpl<DataT, HashedID, BackendT>;
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
template <class Config, class DeprecatedItems>
class ConfigStore : public Config {
    static constexpr bool HAS_DEPRECATED = aggregate_arity<DeprecatedItems>::size() > 1;

    void dump_items() {

        std::apply([](auto &&...args) {
            (args.ram_dump(), ...);
        },
            to_tie(*static_cast<Config *>(this)));
    }

public:
    /**
     * @brief Loads data from a byte array to a hash - handles migrations if the hash was deprecated already.
     * It's meant to be called only by migrating functions.
     *
     * @param id Hashed id of target store item
     * @param buffer Holds incoming data, can be invalidated by the functions
     * @param used_bytes Number of bytes wanted bytes in the buffer
     */
    bool load_item(uint16_t id, std::array<uint8_t, 512> &buffer, uint16_t used_bytes) {
        using TupleT = typename std::invoke_result<decltype(to_tie<Config>), Config &>::type;
        auto constexpr indices = get_current_indices<Config>();
        std::span<uint8_t> span(buffer.data(), used_bytes);
        bool migrated = false;

        while (true) {
            // try to find item in current items
            auto res = std::lower_bound(indices.cbegin(), indices.cend(), detail::CurrentItemIndex<TupleT>(id, nullptr), [](const auto &a, const auto &b) { return a.id < b.id; });
            if (res == indices.cend() || res->id != id) {
                // if not found try to find it in deprecated
                if constexpr (HAS_DEPRECATED) {
                    auto constexpr indices_of_deprecated = get_deprecated_indices<DeprecatedItems>();
                    auto deprecated = std::lower_bound(indices_of_deprecated.cbegin(), indices_of_deprecated.cend(), detail::DeprecatedItemIndex { id, nullptr }, [](const auto &a, const auto &b) { return a.id < b.id; });
                    if (deprecated == indices_of_deprecated.cend() || deprecated->id != id) {
                        // not deprecated and not current -> just return and do nothing
                        return false;
                    }
                    // migrate item from old item to new item and try to find the id
                    std::tie(id, span) = deprecated->fnc(buffer, used_bytes);
                    migrated = true;
                    continue;
                } else {
                    return false;
                }
            }
            // load data to the item itself
            auto tuple = to_tie(*static_cast<Config *>(this));
            res->fnc(span, tuple);
            return migrated;
        }
    }

    void save_all() {
        dump_items();
    };

    void load_all() {
        Config::get_backend().load_all([this](uint16_t id, std::array<uint8_t, 512> &buffer, uint16_t used_bytes) -> bool { return load_item(id, buffer, used_bytes); });
    };
    void init() {
        Config::get_backend().init([this]() {
            dump_items();
        });
    }

    ConfigStore() = default;
    ConfigStore(const ConfigStore &other) = delete;
    ConfigStore(ConfigStore &&other) = delete;
    ConfigStore &operator=(const ConfigStore &other) = delete;
    ConfigStore &operator=(ConfigStore &&other) = delete;
};

template <class Config, class DeprecatedItems>
inline ConfigStore<Config, DeprecatedItems> &journal() {
    static ConfigStore<Config, DeprecatedItems> store {};
    return store;
}
}
