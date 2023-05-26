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

constexpr uint16_t hash(std::span<const char> name) {
    auto hash = cthash::simple<cthash::sha256>(name);
    auto slice = static_cast<uint16_t>((hash[0] << 8) | hash[1]);
    return slice & 0x3FF;
}

template <BackendC BackendT, BackendT &(*backend)()>
struct CurrentStoreConfig {
    static BackendT &get_backend() { return backend(); };
    using Backend = BackendT;
    template <std::default_initializable DataT, DataT default_val, uint16_t id>
    using StoreItem = JournalItem<DataT, default_val, BackendT, backend, id>;
};

template <BackendC BackendT>
struct DeprecatedStoreConfig {
    template <typename DataT, DataT default_val, uint32_t HashedID, auto ptr>
    using DeprecatedStoreItem
        = DeprecatedJournalStoreItemImpl<DataT, HashedID, BackendT,
            remove_member_pointer_t<decltype(ptr)>, extract_class_type_t<decltype(ptr)>, ptr>;
};

template <class T, class U>
static auto consteval has_unique_items() {
    using TupleT = std::invoke_result<decltype(detail::to_tie<T>), T &>::type;
    using TupleU = std::invoke_result<decltype(detail::to_tie<U>), U &>::type;

    using Tuple = decltype(std::tuple_cat(std::declval<TupleT>(), std::declval<TupleU>()));
    constexpr auto index = detail::get_current_indices<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>> {});
    for (size_t i = 0; i < index.size(); i++) {
        for (size_t j = i + 1; j < index.size(); j++) {
            if (index[i].id == index[j].id) {
                consteval_assert_false("Some newly added Ids cause collision");
            }
        }
        for (auto reserved : T::Backend::RESERVED_IDS) {
            if (index[i].id == reserved) {
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
    static_assert(has_unique_items<Config, DeprecatedItems>(), "Just added items are causing collisions");
    static constexpr bool HAS_DEPRECATED = aggregate_arity<DeprecatedItems>::size() > 1;

    void dump_items() {

        std::apply([](auto &&... args) {
            (args.ram_dump(), ...);
        },
            detail::to_tie(*static_cast<Config *>(this)));
    }
    friend void migrate();

    bool load_item(uint16_t id, std::array<uint8_t, 512> &buffer, uint16_t used_bytes) {
        using TupleT = std::invoke_result<decltype(detail::to_tie<Config>), Config &>::type;
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
            auto tuple = detail::to_tie(*static_cast<Config *>(this));
            res->fnc(span, tuple);
            return migrated;
        }
    }

public:
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
};

template <class Config, class DeprecatedItems>
inline ConfigStore<Config, DeprecatedItems> &journal() {
    static ConfigStore<Config, DeprecatedItems> store;
    return store;
}
}
