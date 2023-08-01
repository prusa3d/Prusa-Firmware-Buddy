#include "FUSB302B.hpp"

namespace buddy::hw {

uint8_t FUSB302B::address = 0;

void FUSB302B::InitChip() {

    DetectAddress();

    uint8_t _enable_power[2] = { 0x0B, 0x0F };     // wake up circuit
    uint8_t _enable_interupts[2] = { 0x06, 0x05 }; // enable all interrupts
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _enable_power, 2, 100);
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _enable_interupts, 2, 100);
}

FUSB302B::VBUS_state FUSB302B::ReadSTATUS0Reg() {
    uint8_t _data_form_chip = 0x00;
    uint8_t _data_to_chip[1] = { 0x40 };
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _data_to_chip, 1, 100);
    (void)i2c::Receive(I2C_HANDLE_FOR(usbc), address, &_data_form_chip, 1, 100);
    return static_cast<VBUS_state>(_data_form_chip);
}

void FUSB302B::DetectAddress() {
    // FUSB302B01MPX or FUSB302BMPX is used, depending on board revision. They have different addresses, so try both and detect what chip we have.
    const uint8_t address_options[] = { 0x22 << 1, 0x23 << 1 };
    for (auto tryAddress : address_options) {
        // reset chip, if chip has this address, it will confirm the transaction
        uint8_t _sw_reset[2] = { 0x0C, 0x03 };
        auto res = i2c::Transmit(I2C_HANDLE_FOR(usbc), tryAddress | WRITE_FLAG, _sw_reset, 2, 100);
        if (res == i2c::Result::ok) {
            FUSB302B::address = tryAddress;
            return;
        }
    }
    bsod("FUSB302B address detect fail");
}
}
