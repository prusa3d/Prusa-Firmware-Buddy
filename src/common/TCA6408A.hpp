#pragma once
#include "Pin.hpp"
#include "i2c.hpp"
#include <utility_extensions.hpp>

namespace buddy::hw {

/**
 * @brief This controls TCA6408A I2C IO expander
 */
class TCA6408A {
public:
    enum class Register_t : uint8_t {
        Input = 0,
        Output = 1,
        Polarity = 2,
        Config = 3,
    };

    // timeout for read/write operations
    static constexpr uint32_t timeout_ms = 5;
    static constexpr uint8_t fixed_addr = 0x20;

    constexpr TCA6408A(I2C_HandleTypeDef &i2c)
        : i2c(i2c)
        , read_address((fixed_addr << 1) | 1)
        , write_address(fixed_addr << 1) {}

    /** @brief  Configure given pin as output
     *  Output register - on power on all pins are configured as input pins (1)
     *  Setting them to 0, configures them as output pins
     */
    void configure_all_pins_as_outputs() {
        write_reg(Register_t::Config, config_register_to_outputs);
    }

    bool check_config_reg() {
        uint8_t value = 0xFF;
        bool read_res = read_reg(Register_t::Config, value);
        return read_res && value == config_register_to_outputs;
    }

    /** @brief Write to i2c expander.
     *  It writes to Output register. Then it reads from the configuration register to see if needed pins are set up as outputs. If not, it sets them up.
     *  @param value
     *  @return if write operation was successful (and also setting configuration register was successful)
     */
    bool write(uint8_t value) {
        // value contains a byte, in which each bit represents HIGH / LOW of respective output pin
        bool write_res = write_reg(Register_t::Output, value);

        if (!write_res) {
            return false;
        }

        // Check if pins are set up as outputs
        if (!check_config_reg()) {
            configure_all_pins_as_outputs();
        }

        return true;
    }

private:
    I2C_HandleTypeDef &i2c;
    const uint8_t read_address; //< I2C address for reading registers
    const uint8_t write_address; //< I2C address for writing registers

    static constexpr uint8_t config_register_to_outputs = 0; // configuration register == 0 -> all pins set to write

    /** @brief Read from register.
     *  First it sends write with register address to read from, then it reads from that register.
     *  @param reg [in] register to read from
     *  @param value [out]
     *  @return if read was successful
     */
    bool read_reg(Register_t reg, uint8_t &value) {
        i2c::Result res_transmit = i2c::Transmit(i2c, write_address, (uint8_t *)&reg, sizeof(uint8_t), timeout_ms);
        HAL_Delay(5);
        i2c::Result res_receive = i2c::Receive(i2c, read_address, &value, sizeof(uint8_t), timeout_ms);

        return res_transmit == i2c::Result::ok && res_receive == i2c::Result::ok;
    }

    /** @brief Write to register.
     *  Write a byte to a given register.
     *  @param reg [in] register to write to
     *  @param value [in]
     *  @return if write was successful
     */

    bool write_reg(Register_t reg, uint8_t value) {
        uint8_t data[2] = { (uint8_t)reg, value };
        i2c::Result res = i2c::Transmit(i2c, write_address, data, sizeof(data), timeout_ms);
        return res == i2c::Result::ok;
    }
};
} // namespace buddy::hw
