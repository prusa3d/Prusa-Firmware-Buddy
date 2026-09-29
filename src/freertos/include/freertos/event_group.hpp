/// @file
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <freertos/config.hpp>

namespace freertos {

/// C++ wrapper for FreeRTOS event group.
class EventGroup {
public:
    // We use erased storage in order to not pollute the scope with FreeRTOS internals.
    // The actual size and alignment are statically asserted in implementation file.
    using Storage = std::array<std::byte, event_group_storage_size>;

    /// Only the low 24 bits are usable, rest are reserved.
    using Bits = uint32_t;

private:
    void *handle;
    alignas(event_group_storage_align) Storage event_group_storage;

public:
    EventGroup();

    /// Must not be called while a task waits on it or a _from_isr operation is pending.
    ~EventGroup();

    EventGroup(const EventGroup &) = delete;
    EventGroup &operator=(const EventGroup &) = delete;

    /// Set the bits and return the value they had after underlying function returned.
    Bits set(Bits bits);

    /// Set the bits from interrupt, deferring the actual operation to timer service.
    /// If timer service queue is full, this fails and returns false.
    [[nodiscard]] bool set_from_isr(Bits bits);

    /// Clear the bits and return the value before they were cleared.
    Bits clear(Bits bits);

    /// Clear the bits from interrupt, deferring the actual operation to timer service.
    /// If timer service queue is full, this fails and returns false.
    [[nodiscard]] bool clear_from_isr(Bits bits);

    /// Return the current value of the bits.
    [[nodiscard]] Bits get() { return clear(0); }

    /// Return the current value of the bits from interrupt.
    /// Operations deferred by _from_isr methods may not be reflected yet.
    [[nodiscard]] Bits get_from_isr();

    /// Block until all of the bits are set.
    /// Return the value of the bits when the wait ended, before they were optionally cleared.
    [[nodiscard]] Bits wait_for_all(Bits bits, bool clear_on_exit);

    /// Block until any of the bits is set.
    /// Return the value of the bits when the wait ended, before they were optionally cleared.
    [[nodiscard]] Bits wait_for_any(Bits bits, bool clear_on_exit);

    /// Similar to wait_for_all, but block at most timeout_ms.
    /// On timeout, nothing is cleared and the result may not contain requested bits.
    [[nodiscard]] Bits try_wait_for_all(Bits bits, bool clear_on_exit, uint32_t timeout_ms);

    /// Similar to wait_for_any, but block at most timeout_ms.
    /// On timeout, nothing is cleared and the result may not contain requested bits.
    [[nodiscard]] Bits try_wait_for_any(Bits bits, bool clear_on_exit, uint32_t timeout_ms);
};

} // namespace freertos
