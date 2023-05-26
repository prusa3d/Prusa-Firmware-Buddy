#pragma once
#include <stdint.h>
#include "freertos_mutex.hpp"
#include <span>
#include <functional>
#include "concepts.hpp"
#include "include/dwarf_errors.hpp"
namespace Journal {
template <class DataT, DataT default_val, Journal::BackendC BackendT, BackendT &(*backend)(), uint16_t HashedID>
requires std::equality_comparable<DataT> &&std::default_initializable<DataT> struct JournalItem {
private:
    DataT data { default_val };

public:
    static constexpr uint16_t hashed_id { HashedID };
    static constexpr size_t data_size { sizeof(DataT) };
    static_assert(data_size < BackendT::MAX_ITEM_SIZE, "Item is too large");
    using type = DataT;

    DataT get() {
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

    void init(DataT in) {
        data = std::move(in);
        static_assert(sizeof(*this) == sizeof(DataT), "Current implementation requires the item to be the same size as data it holds");
    }
    void ram_dump() {
        if (data != default_val) {
            do_save();
        }
    }

private:
    void do_save() {
        std::array<uint8_t, sizeof(DataT)> buffer;
        memcpy(buffer.data(), std::launder(reinterpret_cast<const uint8_t *>(&data)), buffer.size());
        backend().save(hashed_id, buffer);
    }
};

template <class DataT, uint16_t HashedID, Journal::BackendC BackendT, typename T, typename CT, T CT::*ptr>
requires std::constructible_from<typename T::type, DataT> struct DeprecatedJournalStoreItemImpl {
    static constexpr uint16_t hashed_id { HashedID };
    static constexpr size_t data_size { sizeof(DataT) };

    static_assert(data_size < BackendT::MAX_ITEM_SIZE, "Item is too large");
    using type = DataT;
    using next_type = T;
    void init(DataT in) {
        data = std::move(in);
    }

private:
    DataT data;
};
}
