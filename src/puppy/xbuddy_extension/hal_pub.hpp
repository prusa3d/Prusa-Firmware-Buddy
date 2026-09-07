///@file
#pragma once

#include <cstdint>
#include <utils/byte_utils.hpp>

namespace hal {

namespace pub {

    /**
     * Initialize PUB (Prusa Universal Bus) subsystem.
     * This must be called while still in privileged mode,
     * because it needs to setup interrupts.
     */
    void init();

    /** This is a CAN-FD identifier. */
    using Identifier = uint32_t;

    /** Transmit a data frame. Doesn't block. May fail if TX FIFO is full. */
    bool transmit(Identifier, Bytes);

    /** Structure for receiving entire frame. */
    struct RxFrame {
        /** CAN-FD identifier */
        Identifier identifier;

        /** Size in bytes, not Data Length Code. */
        size_t size;

        /** Frame data; we are using CAN-FD, so up to 64 bytes of payload. */
        uint8_t data[64];
    };

    /** Receive a frame. Doesn't block. May fail if RX FIFO is empty. */
    bool receive(RxFrame &);

    /** Set or reset the enable pin of the power switch. */
    void enable_pin_set(bool);

    /** Get status of the nfault pin of the power switch. */
    bool nfault_pin_get();

} // namespace pub

} // namespace hal
