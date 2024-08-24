#pragma once

#include "i2c.hpp"
#include <optional>

namespace buddy::hw {

/**
 * @brief This controls TCA6408A I2C IO expander
 */
class TCA6408A {
public:
    enum class Register : uint8_t {
        Input = 0,
        Output = 1,
        Polarity = 2,
        Config = 3,
    };

    static constexpr uint8_t pin_count = 8;
    static constexpr uint8_t fixed_addr = 0x20;

    constexpr TCA6408A(I2C_HandleTypeDef &i2c)
        : i2c(i2c)
        , read_address((fixed_addr << 1) | read_operation)
        , write_address(fixed_addr << 1) {
    }

    /** @brief Set up selected pins on the i2c expander. Other pins will remain unchanged.
     *  It writes to Output register and afterwards it sets up output pins in Configuration Register (according to pin_mask).
     *  @param value will be written to the Output Register
     *  @param pin_mask only bits represented in this mask will be written
     *  @retval true on successful write (to Output Reg & Config Reg), false otherwise
     */
    bool write(uint8_t value, uint8_t pin_mask = 0xFF);

    /** @brief Reads selected pins from Input Register.
     *  @param pin_mask only bits represented in this mask will be read (bits correspond to pins)
     *  @return byte, read from the Input Register (bits outside of pin_mask are 0s)
     *  @return std::nullopt, on failure
     */
    std::optional<uint8_t> read(uint8_t pin_mask = 0xFF) const;

    /** @brief Checks expander's register and update it to match expected value.
     *  @param reg Register to be updated
     *  @param value each bit corresponds to a pin, setting a pin to 1 == input, 0 == output
     *  @param pin_mask Only bits (pins) in pin_mask will be check / set
     *  @return true on successful write to register, false otherwise
     */
    bool update_register(Register reg, uint8_t value, uint8_t pin_mask = 0xFF);

    /** @brief Read from register.
     *  First it sends write with register address to read from, then it reads from that register.
     *  @param reg [in] register to read from
     *  @param value [out]
     *  @return if read was successful
     */
    bool read_reg(Register reg, uint8_t &value) const;

    /** @brief Write to register.
     *  Write a byte to a given register.
     *  @param reg [in] register to write to
     *  @param value [in]
     *  @return if write was successful
     */
    bool write_reg(Register reg, uint8_t value);

    /** @brief Initialize expander's Config Register and Output Register from EEPROM once after power up.
     */
    void initialize();

    /** @brief Toggles selected pin in Output Register.
     */
    bool toggle(uint8_t pin_number);

private:
    I2C_HandleTypeDef &i2c; ///< I2C communication channel
    const uint8_t read_address; ///< I2C address for reading registers
    const uint8_t write_address; ///< I2C address for writing registers

    static constexpr uint8_t config_reg_all_outputs = 0; ///< all pins set to write
    static constexpr uint8_t config_reg_all_inputs = 0xFF; ///< all pins set to read

    // timeout for read/write operations
    static constexpr uint32_t timeout_ms = 5;

    static constexpr uint8_t write_operation = 0;
    static constexpr uint8_t read_operation = 1;

    uint8_t triggered_states = 0xFF; ///< Button states used to avoid multiple triggering on one press; HIGH means not pressed
    bool is_intialized = false;
};
} // namespace buddy::hw
