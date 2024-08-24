#include "TCA6408A.hpp"
#include <config_store/store_instance.hpp>

using namespace buddy::hw;

bool TCA6408A::update_register(Register reg, uint8_t value, uint8_t pin_mask) {
    if (reg == Register::Input) {
        return false;
    }
    uint8_t curr_reg = value;

    if (pin_mask != 0xFF) {
        // Setting only part of the byte
        switch (reg) {
        case Register::Config:
            curr_reg = config_store().io_expander_config_register.get();
            break;
        case Register::Output:
            curr_reg = config_store().io_expander_output_register.get();
            break;
        case Register::Polarity:
            curr_reg = config_store().io_expander_polarity_register.get();
            break;
        case Register::Input:
            assert(false);
            break;
        }

        if ((curr_reg & pin_mask) == (value & pin_mask)) {
            return true;
        }
        // Set up pins selected by pin_mask in config register, other pins remain the unchanged
        curr_reg = (curr_reg & ~pin_mask) | (value & pin_mask);
    }

    if (!write_reg(reg, curr_reg)) {
        return false;
    }

    switch (reg) {
    case Register::Config:
        config_store().io_expander_config_register.set(curr_reg);
        break;
    case Register::Output:
        config_store().io_expander_output_register.set(curr_reg);
        break;
    case Register::Polarity:
        config_store().io_expander_polarity_register.set(curr_reg);
        break;
    case Register::Input:
        break;
    }

    return true;
}

bool TCA6408A::write(uint8_t value, uint8_t pin_mask) {
    if ((config_store().io_expander_config_register.get() & pin_mask) != (config_reg_all_outputs & pin_mask)) {
        SERIAL_ERROR_MSG(" IO Expander - Selected pin is not configured as Output pin. Writing on it may not work as intended.");
    }

    return update_register(Register::Output, value, pin_mask);
}

std::optional<uint8_t> TCA6408A::read(uint8_t pin_mask) const {
    if ((config_store().io_expander_config_register.get() & pin_mask) != (config_reg_all_inputs & pin_mask)) {
        SERIAL_ERROR_MSG(" IO Expander - Used pin(s) is not configured as Input pin. Reading it may produce misleading data.");
    }

    uint8_t value;
    if (read_reg(Register::Input, value)) {
        return value & pin_mask; // If user want to read only selected pins, other pins are set to 0
    }
    return std::nullopt;
}

bool TCA6408A::read_reg(Register reg, uint8_t &value) const {
    i2c::Result res_transmit = i2c::Transmit(i2c, write_address, (uint8_t *)&reg, sizeof(uint8_t), timeout_ms);
    HAL_Delay(5); // TODO: Check if delay can be shortened
    i2c::Result res_receive = i2c::Receive(i2c, read_address, &value, sizeof(uint8_t), timeout_ms);

    return res_transmit == i2c::Result::ok && res_receive == i2c::Result::ok;
}

bool TCA6408A::write_reg(Register reg, uint8_t value) {
    uint8_t data[2] = { (uint8_t)reg, value };
    i2c::Result res = i2c::Transmit(i2c, write_address, data, sizeof(data), timeout_ms);
    return res == i2c::Result::ok;
}

void TCA6408A::initialize() {
    if (is_intialized) {
        return;
    }

    const bool res_output = write_reg(Register::Output, config_store().io_expander_output_register.get());
    const bool res_config = write_reg(Register::Config, config_store().io_expander_config_register.get());
    const bool res_polarity = write_reg(Register::Polarity, config_store().io_expander_polarity_register.get());

    is_intialized = res_output && res_config && res_polarity;
}

bool TCA6408A::toggle(uint8_t pin_number) {
    uint8_t output_reg;
    if (!read_reg(Register::Output, output_reg)) {
        return false;
    }
    output_reg ^= (0x1 << pin_number);
    if (write_reg(Register::Output, output_reg)) {
        config_store().io_expander_output_register.set(output_reg);
        return true;
    }
    return false;
}
