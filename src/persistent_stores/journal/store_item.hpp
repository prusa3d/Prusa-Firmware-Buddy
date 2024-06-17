#pragma once
#include <stdint.h>
#include <span>
#include <exception>
#include "concepts.hpp"
#include "include/dwarf_errors.hpp"
#include <common/array_extensions.hpp>
#include <common/freertos_mutex.hpp>

#pragma GCC push_options
#pragma GCC optimize("Os")

namespace journal {
template <typename DataT>
concept StoreItemDataC = std::equality_comparable<DataT> && std::default_initializable<DataT> && std::is_trivially_copyable_v<DataT>;

template <StoreItemDataC DataT, auto backend>
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
        if (raw_data.size() != sizeof(value_type)) {
            std::terminate();
        }

        memcpy(&data, raw_data.data(), sizeof(value_type));
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
        backend().save(hashed_id, { reinterpret_cast<const uint8_t *>(&data), sizeof(DataT) });
    }
};

template <StoreItemDataC DataT, DataT DefaultVal, auto &(*backend)(), uint16_t HashedID>
struct JournalItem : public JournalItemBase<DataT, backend> {

public:
    static constexpr DataT default_val { DefaultVal };
    static constexpr uint16_t hashed_id { HashedID };

    using Base = JournalItemBase<DataT, backend>;

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

template <StoreItemDataC DataT, DataT DefaultVal, journal::BackendC BackendT, uint16_t HashedID>
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
