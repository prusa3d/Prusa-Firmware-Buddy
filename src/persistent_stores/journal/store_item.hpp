#pragma once
#include <stdint.h>
#include <span>
#include <functional>
#include "concepts.hpp"
#include "include/dwarf_errors.hpp"
#include <common/array_extensions.hpp>
#include <common/freertos_mutex.hpp>

#pragma GCC push_options
#pragma GCC optimize("Os")

namespace journal {
template <typename DataT>
concept StoreItemDataC = std::equality_comparable<DataT> && std::default_initializable<DataT> && std::is_trivially_copyable_v<DataT>;

template <StoreItemDataC DataT, const DataT &DefaultVal, auto &(*backend)(), uint16_t HashedID>
struct JournalItem {
private:
    DataT data { DefaultVal };

public:
    // Obtain backend type based on the return type of the backend (instance) function. Not a part of Item signature to reduce text bloat when examining elf files for optimization purposes
    using BackendT = std::remove_cvref_t<std::invoke_result_t<decltype(backend)>>;
    static_assert(journal::BackendC<BackendT>); // BackendT type needs to fulfill this concept. Can be moved to signature with newer clangd, causes too many errors now (constrained auto)

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

    /// Sets the config to the provided value \p in
    /// \returns true if the set value was different from the previous one
    void set(const DataT &in) {
        if (in == data) {
            return;
        }
        auto l = backend().lock();
        data = in;
        do_save();
    }

    /// Sets the item to f(old_value).
    /// This is done under the lock, so the operation is atomic
    void transform(std::invocable<DataT> auto f) {
        auto l = backend().lock();
        const auto old_value = data;
        const auto new_value = f(old_value);
        if (new_value != old_value) {
            data = new_value;
            do_save();
        }
    }

    /// Sets the config item to its default value.
    /// \returns the default value
    DataT set_to_default() {
        set(default_val);
        return default_val;
    }

    /**
     * @brief Set overload when DataT is holding a string, ie std::array<char, ...>.
     */
    void set(const char *in_buff, size_t max_length = std::tuple_size_v<DataT>)
        requires is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>
    {
        DataT arr_buff;
        strlcpy(arr_buff.data(), in_buff, std::min(arr_buff.size(), max_length));
        set(arr_buff);
    }

    /**
     * @brief Get overload when DataT is holding a string, ie std::array<char, ...>.
     */
    const char *get_c_str()
        requires is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>
    {
        if (xPortIsInsideInterrupt()) {
            return data.data();
        }

        auto l = backend().lock();
        return data.data();
    }

    void init(const DataT &in) {
        data = in;
    }

    void ram_dump() {
        if (data != default_val) {
            do_save();
        }
    }

    JournalItem()
        requires(sizeof(JournalItem) == sizeof(DataT)) // Current implementation of journal relies heavily on this
    = default;
    JournalItem(const JournalItem &other) = delete;
    JournalItem(JournalItem &&other) = delete;
    JournalItem &operator=(const JournalItem &other) = delete;
    JournalItem &operator=(JournalItem &&other) = delete;

private:
    void do_save() {
        backend().save(hashed_id, { reinterpret_cast<const uint8_t *>(&data), sizeof(DataT) });
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

#pragma GCC pop_options
