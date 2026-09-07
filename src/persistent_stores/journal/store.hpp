#pragma once

/**
 * @brief Journal storing strategy means that items are stored persistently using a journal. Changes to item value are stored as an entry to a journal, and upon device reset this journal is read from the beginning, adjusting individual items as the history is being read. Specific implementation may vary (see journal/backend for the one used).
 */

#include <span>

#include "store_config.hpp"
#include <common/visit_all_struct_fields.hpp>

namespace journal {

/**
 * This class takes Config class as template parameter and it defines the items in ConfigStore, DeprecatedItems class has definition for deprecated items.
 */
template <class Config, class DeprecatedItems, const std::span<const journal::Backend::MigrationFunction> &MigrationFunctions>
class Store : public Config {

public:
    /**
     * @brief Loads data from a byte array with a hashed_id.
     * It's meant to be called only by migrating functions.
     *
     * @param id Hashed id of target store item
     * @param data Holds data in binary form to be loaded into current item
     */
    void load_item(uint16_t id, const Bytes &data) {
        visit_all_struct_fields(static_cast<Config &>(*this), [&]<typename Item>(Item &item) {
            item.check_init(id, data);
        });
    }

    void dump_items(ItemFlags exclude_flags = {}) {
        static constexpr auto callback = [](auto &item, auto exclude_flags) {
            item.ram_dump(exclude_flags);
        };
        visit_all_struct_fields(static_cast<Config &>(*this), callback, exclude_flags);
    }

    void save_all() {
        dump_items();
    };

    void load_all() {
        Config::get_backend().load_all([this](uint16_t id, const Bytes &data) -> void { return load_item(id, data); }, MigrationFunctions);
    };
    void init() {
        Config::get_backend().init([this]() {
            dump_items();
        });
    }

    consteval Store()
        : Config {} {}

    Store(const Store &other) = delete;
    Store(Store &&other) = delete;
    Store &operator=(const Store &other) = delete;
    Store &operator=(Store &&other) = delete;
};

} // namespace journal
