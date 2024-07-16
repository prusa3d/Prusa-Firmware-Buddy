#pragma once

#include <array>
#include <algorithm>

enum class DynamicIndexMappingType {
    /// Single static item that is always in the list
    static_item,

    /// Section of multiple items, size is defined in runtime
    dynamic_section,

    /// Section of multiple item of fixed size
    static_section = static_item,

    /// Single item that might or might not be in the list. On by default
    optional_item = dynamic_section,
};

template <class Item_>
struct DynamicIndexMappingRecord {

public:
    using Item = Item_;

public:
    constexpr DynamicIndexMappingRecord(Item item, DynamicIndexMappingType type = DynamicIndexMappingType::static_item, size_t section_size = 1)
        : item(item)
        , type(type)
        , section_size(section_size) {}

public:
    /// Enum value corresponding to the item
    Item item;

    /// Item type
    DynamicIndexMappingType type = DynamicIndexMappingType::static_item;

    /// Default/static section size (or optional item)
    size_t section_size = 1;

public:
    inline constexpr bool is_dynamic() const {
        return type == DynamicIndexMappingType::dynamic_section;
    }
};

/// Utility class for mapping enum item <> index in list
/// See dynamic_index_mapping_tests for usage examples
template <auto items>
class DynamicIndexMapping {

public:
    using Items = decltype(items);
    using ItemRecord = typename Items::value_type;
    using Item = typename ItemRecord::Item;

    struct FromIndexResult {
        Item item;

        /// If \p item is a section, this stores index in the section
        size_t pos_in_section = 0;

        constexpr inline bool operator==(const FromIndexResult &) const = default;
        constexpr inline bool operator!=(const FromIndexResult &) const = default;
    };

public:
    /// \returns size of a dynamic section
    template <Item item>
    size_t section_size() const {
        static constexpr auto record = item_record<item>();
        static_assert(record.type == DynamicIndexMappingType::dynamic_section);
        return dynamic_section_sizes[dynamic_section_count_before<item>()];
    }

    /// Sets size of a dynamic section
    template <Item item>
    void set_section_size(size_t set) {
        static constexpr auto record = item_record<item>();
        static_assert(record.type == DynamicIndexMappingType::dynamic_section);
        dynamic_section_sizes[dynamic_section_count_before<item>()] = set;
    }

    /// \returns whether an item is enabled
    template <Item item>
    bool is_item_enabled() const {
        static constexpr auto record = item_record<item>();
        static_assert(record.type == DynamicIndexMappingType::optional_item);
        return dynamic_section_sizes[dynamic_section_count_before<item>()] != 0;
    }

    /// Sets whether an optional item should be shown or not
    template <Item item>
    void set_item_enabled(bool set) {
        static constexpr auto record = item_record<item>();
        static_assert(record.type == DynamicIndexMappingType::optional_item);
        dynamic_section_sizes[dynamic_section_count_before<item>()] = set ? 1 : 0;
    }

    /// \returns total item count
    constexpr size_t total_item_count() const {
        return total_static_item_count + std::accumulate(dynamic_section_sizes.begin(), dynamic_section_sizes.end(), 0);
    }

    /// \returns index of \p item
    /// \param pos_in_section index in the section, if \p item is a section
    template <Item item>
    constexpr size_t to_index(size_t pos_in_section = 0) const {
        return static_item_count_before<item>() + dynamic_item_count_before<item>() + pos_in_section;
    }

    constexpr FromIndexResult from_index(size_t index) const {
        return [&]<size_t... ix>(std::index_sequence<ix...>) {
            FromIndexResult result {
                .item = static_cast<Item>(0),
                .pos_in_section = index,
            };

            const auto accum_item = [&]<ItemRecord item>() -> bool {
                const size_t s_size = section_size<item>();
                if (result.pos_in_section < s_size) {
                    result.item = item.item;
                    return true;
                } else {
                    result.pos_in_section -= s_size;
                    return false;
                }
            };

            (accum_item.template operator()<items[ix]>() || ...);

            return result;
        }(std::make_index_sequence<items.size()>());
    }

protected:
    static constexpr size_t dynamic_section_count = std::count_if(items.begin(), items.end(), [](const auto &i) { return i.is_dynamic(); });
    static constexpr size_t total_static_item_count = std::accumulate(items.begin(), items.end(), 0, [](const auto &s, const auto &i) { return s + (i.is_dynamic() ? 0 : i.section_size); });

    /// \returns iterator of the provided item
    template <Item item>
    static constexpr inline typename Items::const_iterator item_iterator() {
        constexpr auto result = std::find_if(items.begin(), items.end(), [](const auto &i) { return i.item == item; });
        return result;
    }

    /// \returns number of dynamic sections before the provided item
    template <Item item>
    static constexpr size_t dynamic_section_count_before() {
        static constexpr typename Items::const_iterator end = item_iterator<item>();
        static constexpr size_t result = std::count_if(items.begin(), end, [](const auto &i) { return i.is_dynamic(); });
        return result;
    }

    /// \returns sum of dynamic items before the provided item
    template <Item item>
    constexpr size_t dynamic_item_count_before() const {
        return std::accumulate(dynamic_section_sizes.begin(), dynamic_section_sizes.begin() + dynamic_section_count_before<item>(), 0);
    }

    /// \returns sum of static items before the provided item
    template <Item item>
    static constexpr inline size_t static_item_count_before() {
        static constexpr auto accum_f = [](size_t acc, const auto &i) { return acc + (i.is_dynamic() ? 0 : i.section_size); };
        static constexpr typename Items::const_iterator end = item_iterator<item>();
        static constexpr size_t result = std::accumulate(items.begin(), end, size_t(0), accum_f);
        return result;
    }

    /// \returns section size of a provided item
    template <ItemRecord item>
    constexpr inline size_t section_size() const {
        if constexpr (item.is_dynamic()) {
            return dynamic_section_sizes[dynamic_section_count_before<item.item>()];
        } else {
            return item.section_size;
        }
    }

    /// \returns item record of the item
    template <Item item>
    static constexpr inline ItemRecord item_record() {
        static constexpr auto result = std::find_if(items.begin(), items.end(), [](const auto &i) { return i.item == item; });
        static_assert(result != items.end());
        return *result;
    }

    static constexpr std::array<size_t, dynamic_section_count> default_dynamic_section_sizes = [] {
        std::array<size_t, dynamic_section_count> result;
        size_t i = 0;
        for (const auto &item : items) {
            if (!item.is_dynamic()) {
                continue;
            }

            result[i++] = item.section_size;
        }
        return result;
    }();

    std::array<size_t, dynamic_section_count> dynamic_section_sizes = default_dynamic_section_sizes;
};
