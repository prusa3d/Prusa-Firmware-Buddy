#pragma once
#include <stdint.h>
#include "freertos_mutex.hpp"
#include <span>
#include <functional>
#include "concepts.hpp"
#include "include/dwarf_errors.hpp"
#include <common/array_extensions.hpp>

namespace journal {
template <typename DataT>
concept StoreItemDataC = std::equality_comparable<DataT> && std::default_initializable<DataT> && std::is_trivially_copyable_v<DataT>;

template <StoreItemDataC DataT, const DataT &DefaultVal, journal::BackendC BackendT, BackendT &(*backend)(), uint16_t HashedID>
struct JournalItem {
private:
    DataT data { DefaultVal };

public:
    static constexpr DataT default_val { DefaultVal };
    static constexpr uint16_t hashed_id { HashedID };
    static constexpr size_t data_size { sizeof(DataT) };
    static_assert(data_size < BackendT::MAX_ITEM_SIZE, "Item is too large");
    using value_type = DataT;

    DataT get() {
        if (xPortIsInsideInterrupt()) {
            return data;
        }

        auto l = backend().lock();
        return data;
    }
    void set(DataT in) {
        if (in == data) {
            return;
        }
        auto l = backend().lock();
        data = std::move(in);
        do_save();
    }

    /**
     * @brief Set overload when DataT is holding a string, ie std::array<char, ...>.
     */
    void set(const char *in_buff, size_t max_length = std::tuple_size_v<DataT>) {
        static_assert(is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>, "Invalid function call");
        DataT arr_buff;
        strlcpy(arr_buff.data(), in_buff, std::min(arr_buff.size(), max_length));
        set(arr_buff);
    }
    /**
     * @brief Get overload when DataT is holding a string, ie std::array<char, ...>.
     */
    const char *get_c_str() {
        static_assert(is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>, "Invalid function call");
        if (xPortIsInsideInterrupt()) {
            return data.data();
        }

        auto l = backend().lock();
        return data.data();
    }

    void init(DataT in) {
        data = std::move(in);
        static_assert(sizeof(*this) == sizeof(DataT), "Current implementation requires the item to be the same size as data it holds");
    }
    void ram_dump() {
        if (data != default_val) {
            do_save();
        }
    }

    JournalItem() = default;
    JournalItem(const JournalItem &other) = delete;
    JournalItem(JournalItem &&other) = delete;
    JournalItem &operator=(const JournalItem &other) = delete;
    JournalItem &operator=(JournalItem &&other) = delete;

private:
    void do_save() {
        std::array<uint8_t, sizeof(DataT)> buffer;
        memcpy(buffer.data(), reinterpret_cast<const uint8_t *>(&data), buffer.size());
        backend().save(hashed_id, buffer);
    }
};

template <StoreItemDataC DataT, const DataT &DefaultVal, journal::BackendC BackendT, uint16_t HashedID>
struct DeprecatedStoreItem {
    static constexpr uint16_t hashed_id { HashedID };
    static constexpr size_t data_size { sizeof(DataT) };
    static constexpr bool is_deleted { true };
    static constexpr DataT default_val { DefaultVal };

    static_assert(data_size < BackendT::MAX_ITEM_SIZE, "Item is too large");
    using value_type = DataT;
};

} // namespace journal
