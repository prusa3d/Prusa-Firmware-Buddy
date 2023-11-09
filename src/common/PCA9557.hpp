#pragma once
#include "Pin.hpp"
#include "i2c.hpp"

namespace buddy::hw {

/// @brief This controls PCA9557 I2C IO extender
class PCA9557 {
public:
    enum class Register_t : uint8_t {
        Input = 0,
        Output = 1,
        Polarity = 2,
        Config = 3,
    };

    // timeout for read/write operations
    static constexpr uint32_t TIMEOUT = 5;

    constexpr PCA9557(I2C_HandleTypeDef &i2c, const uint8_t address)
        : i2c(i2c)
        , read_address(0x30 | (address << 1) | 1)
        , write_address(0x30 | (address << 1)) {}

    /// @brief  Configure pin as output
    /// @param pin
    void configure_as_output(const buddy::hw::IoPin pin) {
        config_register_val &= ~(1 << (uint8_t)pin);
        write_reg(Register_t::Config, config_register_val);
    }

    /// @brief Set output pin HIGH
    /// @param pin
    void output_set(const buddy::hw::IoPin pin) {
        output_register_val |= (1 << (uint8_t)pin);
        write_reg(Register_t::Output, output_register_val);
    }

    /// @brief Set output pin LOW
    /// @param pin
    void output_reset(const buddy::hw::IoPin pin) {
        output_register_val &= ~(1 << (uint8_t)pin);
        write_reg(Register_t::Output, output_register_val);
    }

private:
    I2C_HandleTypeDef &i2c;
    const uint8_t read_address; //< I2C address for reading registers
    const uint8_t write_address; //< I2C address for writing registers

    uint8_t config_register_val = 0xFF; //< local snapshot of configuration register
    uint8_t output_register_val = 0x00; //< local snapshot of output register

    void write_reg(Register_t reg, uint8_t value) {
        uint8_t data[2] = { (uint8_t)reg, value };
        (void)i2c::Transmit(i2c, write_address, data, sizeof(data), TIMEOUT);
    }
};

/// @brief This represents one pin on PCA9557 IO extender, that is configured as output
class PCA9557OutputPin {
public:
    constexpr PCA9557OutputPin(buddy::hw::IoPin pin, Pin::State init_state, PCA9557 &device)
        : device(device)
        , pin(pin)
        , init_state(init_state) {}

    void configure() const {
        if (init_state == Pin::State::low) {
            device.output_reset(pin);
        } else {
            device.output_set(pin);
        }
        device.configure_as_output(pin);
    }

    /// @brief  Set pin state
    /// @param pinState
    void write(Pin::State pinState) const {
        if (pinState == Pin::State::low) {
            reset();
        } else {
            set();
        }
    }

    /// @brief Set pin HIGH
    inline void set() const {
        device.output_set(pin);
    }

    /// @brief Set pin LOW
    inline void reset() const {
        device.output_reset(pin);
    }

    PCA9557 &device; //< chip instance this pin is controller by
    const buddy::hw::IoPin pin; //< pin number
    const Pin::State init_state; //< Initial state (low or high)
};
} // namespace buddy::hw
