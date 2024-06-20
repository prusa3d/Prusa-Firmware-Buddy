#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <array>

namespace freertos {

// C++ wrapper for FreeRTOS queue.
class QueueBase {
public:
// We use erased storage in order to not pollute the scope with FreeRTOS internals.
// The actual size and alignment are statically asserted in implementation file.
#ifdef UNITTESTS
    static constexpr size_t queue_storage_size = 168;
    static constexpr size_t queue_storage_align = 8;
#else
    static constexpr size_t queue_storage_size = 80;
    static constexpr size_t queue_storage_align = 4;
#endif

    using Storage = std::array<uint8_t, queue_storage_size>;

private:
    alignas(queue_storage_align) Storage queue_storage;

protected:
    QueueBase(size_t item_count, size_t item_size, uint8_t *item_storage);
    ~QueueBase();
    QueueBase(const QueueBase &) = delete;
    QueueBase &operator=(const QueueBase &) = delete;
    void send(const void *payload);
    void receive(void *payload);
    [[nodiscard]] bool try_send(const void *payload, size_t milliseconds_to_wait);
    [[nodiscard]] bool try_receive(void *payload, size_t milliseconds_to_wait);
};

// C++ wrapper for FreeRTOS queue and its storage.
template <class T, size_t N>
class Queue final : public QueueBase {
public:
    Queue()
        : QueueBase(N, sizeof(T), item_storage) {}

    void send(const T &payload) {
        QueueBase::send(&payload);
    }

    void receive(T &payload) {
        QueueBase::receive(&payload);
    }

    [[nodiscard]] bool try_send(const T &payload, size_t milliseconds_to_wait) {
        return QueueBase::try_send(&payload, milliseconds_to_wait);
    }

    [[nodiscard]] bool try_receive(T &payload, size_t milliseconds_to_wait) {
        return QueueBase::try_receive(&payload, milliseconds_to_wait);
    }

private:
    uint8_t item_storage[N * sizeof(T)];
};

} // namespace freertos
