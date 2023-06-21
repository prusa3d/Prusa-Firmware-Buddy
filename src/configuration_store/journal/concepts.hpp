#pragma once
#include <functional>
#include "freertos_mutex.hpp"
#include <span>

namespace Journal {
using UpdateFunction = std::function<bool(uint16_t, std::array<uint8_t, 512> &, uint16_t)>;
using DumpCallback = std::function<void(void)>;

template <typename T>
concept BackendC = requires(T &t, uint16_t id, std::span<uint8_t> data, const UpdateFunction &update_function, DumpCallback dump_callback) {
    { t.save(id, data) }
        -> std::same_as<void>;
    { t.lock() }
        -> std::same_as<std::unique_lock<FreeRTOS_Mutex>>;
    { t.load_all(update_function) };
    { T::MAX_ITEM_SIZE };
};
template <typename T>
concept ConfigC = requires() {
    { T::get_backend() };
};

template <typename T>
concept IsDeletedStoreItemC = T::is_deleted;
}
