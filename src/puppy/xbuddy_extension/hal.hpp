///@file
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace hal {

using DutyCycle = uint8_t;

/**
 * Initialize hardware abstraction layer module.
 * This must be called while still in privileged mode,
 * because it needs to setup interrupts.
 */
void init();

/**
 * Enter infinite loop.
 */
[[noreturn]] void panic();

/**
 * Step the HAL subsystem.
 * This blocks and must be called periodically.
 */
void step();

// Each peripheral gets its own namespace

namespace fan1 {
    void set_pwm(DutyCycle duty_cycle);
    uint32_t get_raw();
} // namespace fan1

namespace fan2 {
    void set_pwm(DutyCycle duty_cycle);
    uint32_t get_raw();
} // namespace fan2

namespace fan3 {
    void set_pwm(DutyCycle duty_cycle);
    uint32_t get_raw();
} // namespace fan3

namespace w_led {
    void set_pwm(DutyCycle duty_cycle);
}

namespace rgbw_led {
    void set_r_pwm(DutyCycle duty_cycle);
    void set_g_pwm(DutyCycle duty_cycle);
    void set_b_pwm(DutyCycle duty_cycle);
    void set_w_pwm(DutyCycle duty_cycle);
} // namespace rgbw_led

namespace temperature {
    uint32_t get_raw();
}

namespace rs485 {

    /**
     * Start receiving messages.
     * Does not block.
     */
    void start_receiving();

    /**
     * Blocks until message is received.
     * Returned span is valid until next transmit()
     */
    std::span<std::byte> receive();

    /**
     * Transmit message.
     * Does not block.
     * Supplied span must remain valid until next receive()
     */
    void transmit_and_then_start_receiving(std::span<std::byte>);

} // namespace rs485

namespace mmu {
    /**
     * Transmit bytes on MMU UART.
     * Blocks until all bytes are transmitted.
     */
    void transmit(std::span<const std::byte>);

    /**
     * Receive bytes from MMU UART.
     * Bytes are received into supplied buffer.
     * Returns view into that buffer.
     * Does not block.
     */
    std::span<std::byte> receive(std::span<std::byte>);

    /**
     * Flush the receive buffer, discarding its contents.
     */
    void flush();

} // namespace mmu

} // namespace hal
