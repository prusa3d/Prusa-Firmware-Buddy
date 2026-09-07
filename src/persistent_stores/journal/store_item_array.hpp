#pragma once

#include <freertos/task.hpp>
#include <utils/algorithm_extensions.hpp>
#include <utils/byte_utils.hpp>

#include "concepts.hpp"

#pragma GCC push_options
#pragma GCC optimize("Os")

namespace journal {

template <typename Child, StoreItemDataC DataT, auto default_val_, ItemFlags flags_, auto backend, uint8_t item_count>
struct JournalItemArrayBase {

public:
    using DefaultVal = std::remove_cvref_t<decltype(default_val_)>;
    static constexpr DefaultVal default_val = default_val_;

    using ItemArray = std::array<DataT, item_count>;
    using BackendT = std::remove_cvref_t<std::invoke_result_t<decltype(backend)>>;
    using value_type = DataT;
    using DataArg = JournalItemBase<DataT, backend, false>::DataArg;

    static constexpr ItemFlags flags { flags_ }; // All items have flag 1 (for easier filtering)

    static constexpr size_t data_size { sizeof(DataT) };
    static_assert(journal::BackendC<BackendT>); // BackendT type needs to fulfill this concept. Can be moved to signature with newer clangd, causes too many errors now (constrained auto)
    static_assert(data_size < BackendT::MAX_ITEM_SIZE, "Item is too large");
    static_assert(std::is_same_v<DefaultVal, std::array<DataT, item_count>> || std::is_convertible_v<DefaultVal, DataT>);
    static_assert(item_count > 0);

protected:
    ItemArray data_array;

public:
    static constexpr DataT get_default_val(uint8_t index) {
        if constexpr (is_std_array_v<DefaultVal>) {
            return default_val[index];
        } else {
            return default_val;
        }
    }

    consteval JournalItemArrayBase()
        requires(sizeof(JournalItemArrayBase) == sizeof(ItemArray)) // Current implementation of journal relies heavily on this
    {
        if constexpr (is_std_array_v<DefaultVal>) {
            data_array = default_val;
        } else {
            data_array.fill(default_val);
        }
    }

    JournalItemArrayBase(const JournalItemArrayBase &other) = delete;
    JournalItemArrayBase &operator=(const JournalItemArrayBase &other) = delete;

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
        static_cast<Child *>(this)->do_save(index);
    }

    void set_all(DataArg in) {
        auto l = backend().lock();
        for (size_t i = 0; i < item_count; i++) {
            if (data_array[i] == in) {
                continue;
            }
            data_array[i] = in;
            static_cast<Child *>(this)->do_save(i);
        }
    }

    void set_all(ItemArray &in) {
        auto l = backend().lock();
        for (size_t i = 0; i < item_count; i++) {
            if (data_array[i] == in[i]) {
                continue;
            }
            data_array[i] = in[i];
            static_cast<Child *>(this)->do_save(i);
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
            this->data_array[index] = new_value;
            static_cast<Child *>(this)->do_save(index);
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
                this->data_array[i] = new_value;
                static_cast<Child *>(this)->do_save(i);
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

        if (freertos::is_inside_interrupt()) {
            return data_array[index];
        }

        auto l = backend().lock();
        return data_array[index];
    }

    auto get_all() {
        if (freertos::is_inside_interrupt()) {
            return data_array;
        }

        auto l = backend().lock();
        return data_array;
    }

    void init(uint8_t index, const Bytes &raw_data) {
        if ((raw_data.size() != sizeof(value_type)) || (index >= item_count)) {
            std::terminate();
        }

        memcpy(&(data_array[index]), raw_data.data(), sizeof(value_type));
    }

    void ram_dump(ItemFlags exclude_flags) {
        if (flags & exclude_flags) {
            return;
        }

        for (uint8_t i = 0; i < item_count; i++) {
            if (data_array[i] != get_default_val(i)) {
                static_cast<Child *>(this)->do_save(i);
            }
        }
    }
};

/// Array of journal items
/// \p item_count determines the array size. It can be increased in time, possibly even decreased
/// \p max_item_count determines the maximum item_count the item can ever have. This is only used for hash collision checking. It can never be decreased, but it can be increased (granted that it does not cause hash collisions)
/// The journal_hashes_generator python script looks for the next argument after journal::hash for the hash range size - so \p max_item_count must be directly after \p hashed_id
template <StoreItemDataC DataT, auto default_val, ItemFlags flags_, auto backend, uint16_t hashed_id, uint8_t max_item_count, uint8_t item_count>
struct JournalItemArray : public JournalItemArrayBase<JournalItemArray<DataT, default_val, flags_, backend, hashed_id, max_item_count, item_count>, DataT, default_val, flags_, backend, item_count> {

public:
    static_assert(max_item_count >= item_count);

    static constexpr uint16_t hashed_id_first { hashed_id };
    static constexpr uint16_t hashed_id_last { hashed_id + item_count - 1 };

public:
    inline void check_init(uint16_t id, const Bytes &data) {
        if (hashed_id_first <= id && id <= hashed_id_last) {
            this->init(id - hashed_id_first, data);
        }
    }

    void do_save(uint8_t index) {
        if (index >= item_count) {
            std::terminate();
        }
        backend().save(hashed_id_first + index, trivial_as_bytes(this->data_array[index]));
    }
};

/// Legacy variant of JournalItemArray for items that were arrays before arrays existed.
/// \param hashed_ids list of hashed IDs of the individual items
template <StoreItemDataC DataT, auto default_val, ItemFlags flags_, auto backend, auto hashed_ids>
struct JournalItemLegacyArray : public JournalItemArrayBase<JournalItemLegacyArray<DataT, default_val, flags_, backend, hashed_ids>, DataT, default_val, flags_, backend, hashed_ids.size()> {

public:
    inline void check_init(uint16_t id, const Bytes &data) {
        if (const auto index = stdext::index_of(hashed_ids, id); index != hashed_ids.size()) {
            this->init(index, data);
        }
    }

    void do_save(uint8_t index) {
        if (index >= hashed_ids.size()) {
            std::terminate();
        }
        backend().save(hashed_ids[index], trivial_as_bytes(this->data_array[index]));
    }
};

} // namespace journal

#pragma GCC pop_options
