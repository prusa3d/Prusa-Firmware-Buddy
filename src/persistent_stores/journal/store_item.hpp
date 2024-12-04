#pragma once
#include <stdint.h>
#include <span>
#include <exception>
#include "concepts.hpp"
#include "include/dwarf_errors.hpp"
#include <common/array_extensions.hpp>
#include <freertos/mutex.hpp>
#include "FreeRTOS.h"

#pragma GCC push_options
#pragma GCC optimize("Os")

namespace journal {
template <typename DataT>
concept StoreItemDataC = std::equality_comparable<DataT> && std::default_initializable<DataT> && std::is_trivially_copyable_v<DataT>;

template <StoreItemDataC DataT, auto backend, bool ram_only>
struct JournalItemBase {
protected:
    DataT data;

public:
    // Obtain backend type based on the return type of the backend (instance) function. Not a part of Item signature to reduce text bloat when examining elf files for optimization purposes
    using BackendT = std::remove_cvref_t<std::invoke_result_t<decltype(backend)>>;
    static_assert(journal::BackendC<BackendT>); // BackendT type needs to fulfill this concept. Can be moved to signature with newer clangd, causes too many errors now (constrained auto)

    static constexpr size_t data_size { sizeof(DataT) };
    static_assert(data_size < BackendT::MAX_ITEM_SIZE, "Item is too large");
    using value_type = DataT;

    /// Optimal way of passing the data as a parameter (either const ref or by value, based on type)
    using DataArg = std::conditional_t<(sizeof(DataT) > 4), const DataT &, const DataT>;

    JournalItemBase(const JournalItemBase &other) = delete;
    JournalItemBase &operator=(const JournalItemBase &other) = delete;

    DataT get() {
        if (xPortIsInsideInterrupt()) {
            return data;
        }

        auto l = backend().lock();
        return data;
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

    void init(const std::span<uint8_t> &raw_data) {
        if constexpr (!ram_only) {
            if (raw_data.size() != sizeof(value_type)) {
                std::terminate();
            }

            memcpy(&data, raw_data.data(), sizeof(value_type));
        }
    }

protected:
    constexpr inline JournalItemBase(const DataT &val)
        : data(val) {}

    /// Sets the config to the provided value \p in
    /// \returns true if the set value was different from the previous one
    void set(uint16_t hashed_id, DataArg in) {
        if (in == data) {
            return;
        }
        auto l = backend().lock();
        data = in;
        do_save(hashed_id);
    }

    /**
     * @brief Set overload when DataT is holding a string, ie std::array<char, ...>.
     */
    void set(uint16_t hashed_id, const char *in_buff, size_t max_length = std::tuple_size_v<DataT>)
        requires is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>
    {
        DataT arr_buff;
        strlcpy(arr_buff.data(), in_buff, std::min(arr_buff.size(), max_length));
        set(hashed_id, arr_buff);
    }

    void ram_dump(uint16_t hashed_id, DataArg default_val) {
        if (data != default_val) {
            do_save(hashed_id);
        }
    }

    void do_save(uint16_t hashed_id) {
        if constexpr (!ram_only) {
            backend().save(hashed_id, { reinterpret_cast<const uint8_t *>(&data), sizeof(DataT) });
        }
    }
};

// hash_alloc_range is used by the gen_journal_hashes.py and is expected after journal::hash. The argument is added here to prevent clashes with ram_only
template <StoreItemDataC DataT, auto DefaultVal, auto &(*backend)(), uint16_t HashedID, uint8_t hash_alloc_range, bool ram_only>
struct JournalItem : public JournalItemBase<DataT, backend, ram_only> {
    static_assert(hash_alloc_range >= 1);

public:
    static constexpr DataT default_val { DefaultVal };
    static constexpr uint16_t hashed_id { HashedID };

    using Base = JournalItemBase<DataT, backend, ram_only>;

public:
    constexpr JournalItem()
        requires(sizeof(JournalItem) == sizeof(DataT)) // Current implementation of journal relies heavily on this
        : Base(default_val) {
    }

    /// Sets the config to the provided value \p in
    /// \returns true if the set value was different from the previous one
    inline void set(Base::DataArg in) {
        Base::set(hashed_id, in);
    }

    /// Sets the item to f(old_value).
    /// This is done under the lock, so the operation is atomic
    inline void transform(std::invocable<typename Base::DataArg> auto f) {
        // This class gets instantiated for each f anyway, so no need to put it into base class
        auto l = backend().lock();
        const auto old_value = this->data;
        const auto new_value = f(old_value);
        if (new_value != old_value) {
            this->data = new_value;
            this->do_save(hashed_id);
        }
    }

    /// Applies f(val) on the item value
    /// This is done under the lock, so the operation is atomic
    inline void apply(std::invocable<DataT &> auto f) {
        // This class gets instantiated for each f anyway, so no need to put it into base class
        auto l = backend().lock();
        const auto old_value = this->data;
        DataT new_value = old_value;
        f(new_value);
        if (new_value != old_value) {
            this->data = new_value;
            this->do_save(hashed_id);
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
    inline void set(const char *in_buff, size_t max_length = std::tuple_size_v<DataT>)
        requires is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>
    {
        Base::set(hashed_id, in_buff, max_length);
    }

    inline void ram_dump() {
        Base::ram_dump(hashed_id, default_val);
    }
};

/// Array of journal items
/// \p item_count determines the array size. It can be increased in time, possibly even decreased
/// \p max_item_count determines the maximum item_count the item can ever have. This is only used for hash collision checking. It can never be decreased, but it can be increased (granted that it does not cause hash collisions)
/// The journal_hashes_generator python script looks for the next argument after journal::hash for the hash range size - so \p max_item_count must be directly after \p hashed_id
template <StoreItemDataC DataT, auto default_val, auto backend, uint16_t hashed_id, uint8_t max_item_count, uint8_t item_count>
struct JournalItemArray {
private:
    using DefaultVal = std::remove_cvref_t<decltype(default_val)>;
    using ItemArray = std::array<DataT, item_count>;
    ItemArray data_array;

public:
    using BackendT = std::remove_cvref_t<std::invoke_result_t<decltype(backend)>>;
    using value_type = DataT;
    static constexpr size_t data_size { sizeof(DataT) };
    static_assert(journal::BackendC<BackendT>); // BackendT type needs to fulfill this concept. Can be moved to signature with newer clangd, causes too many errors now (constrained auto)
    static_assert(data_size < BackendT::MAX_ITEM_SIZE, "Item is too large");
    static_assert(max_item_count >= item_count);
    static_assert(std::is_same_v<DefaultVal, std::array<DataT, item_count>> || std::is_same_v<DefaultVal, DataT>);
    static_assert(item_count > 0);

    using DataArg = JournalItemBase<DataT, backend, false>::DataArg;

    static constexpr uint16_t hashed_id_first { hashed_id };
    static constexpr uint16_t hashed_id_last { hashed_id + item_count - 1 };

    static constexpr DataT get_default_val(uint8_t index) {
        if constexpr (is_std_array_v<DefaultVal>) {
            return default_val[index];
        } else {
            return default_val;
        }
    }

    constexpr JournalItemArray()
        requires(sizeof(JournalItemArray) == sizeof(ItemArray)) // Current implementation of journal relies heavily on this
    {
        if constexpr (is_std_array_v<DefaultVal>) {
            data_array = default_val;
        } else {
            data_array.fill(default_val);
        }
    }
    JournalItemArray(const JournalItemArray &other) = delete;
    JournalItemArray &operator=(const JournalItemArray &other) = delete;

    /// Sets the config to the provided value \p in
    /// \returns true if the set value was different from the previous one
    void set(uint8_t index, DataArg in) {
        if (index >= item_count) {
            std::terminate();
        }
        if (data_array[index] == in) {
            return;
        }
        auto l = backend().lock();

        data_array[index] = in;
        do_save(index);
    }

    void set_all(DataArg in) {
        auto l = backend().lock();
        for (size_t i = 0; i < item_count; i++) {
            if (data_array[i] == in) {
                continue;
            }
            data_array[i] = in;
            do_save(i);
        }
    }

    void set_all(ItemArray &in) {
        auto l = backend().lock();
        for (size_t i = 0; i < item_count; i++) {
            if (data_array[i] == in[i]) {
                continue;
            }
            data_array[i] = in[i];
            do_save(i);
        }
    }
    /// Sets the item to f(old_value).
    /// This is done under the lock, so the operation is atomic
    inline void transform(uint8_t index, std::invocable<DataArg> auto f) {
        if (index >= item_count) {
            std::terminate();
        }
        auto l = backend().lock();
        const auto old_value = this->data_array[index];
        const auto new_value = f(old_value);
        if (new_value != old_value) {
            this->data_array = new_value;
            this->do_save(index);
        }
    }

    /// Sets the item to f(old_value).
    /// This is done under the lock, so the operation is atomic
    inline void transform_all(std::invocable<DataArg> auto f) {
        auto l = backend().lock();
        for (uint8_t i = 0; i < item_count; i++) {
            const auto old_value = this->data_array[i];
            const auto new_value = f(old_value);
            if (new_value != old_value) {
                this->data_array = new_value;
                this->do_save(i);
            }
        }
    }

    /// Sets the config item to its default value.
    /// \returns the default value
    inline void set_to_default(uint8_t index) {
        set(index, get_default_val(index));
    }

    /// Sets the config item to its default value.
    /// \returns the default value
    inline void set_all_to_default() {
        set_all(default_val);
    }

    DataT get(uint8_t index) {
        if (index >= item_count) {
            std::terminate();
        }

        if (xPortIsInsideInterrupt()) {
            return data_array[index];
        }

        auto l = backend().lock();
        return data_array[index];
    }

    void init(uint8_t index, const std::span<uint8_t> &raw_data) {
        if ((raw_data.size() != sizeof(value_type)) || (index >= item_count)) {
            std::terminate();
        }

        memcpy(&(data_array[index]), raw_data.data(), sizeof(value_type));
    }

    void ram_dump() {
        for (uint8_t i = 0; i < item_count; i++) {
            if (data_array[i] != get_default_val(i)) {
                do_save(i);
            }
        }
    }

private:
    void do_save(uint8_t index) {
        if (index >= item_count) {
            std::terminate();
        }
        backend().save(hashed_id_first + index, { reinterpret_cast<const uint8_t *>(&(data_array[index])), sizeof(DataT) });
    }
};

template <typename>
struct is_item_array : std::false_type {};
template <typename DataT, auto default_val, auto backend, uint16_t hashed_id, uint8_t max_item_count, uint8_t item_count>
struct is_item_array<JournalItemArray<DataT, default_val, backend, hashed_id, max_item_count, item_count>> : std::true_type {};

template <typename T>
inline constexpr bool is_item_array_v = is_item_array<T>::value;

template <StoreItemDataC DataT, auto DefaultVal, journal::BackendC BackendT, uint16_t HashedID>
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
