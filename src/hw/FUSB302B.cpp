#include "FUSB302B.hpp"
#include "bsod.h"
#include <freertos/critical_section.hpp>

namespace buddy::hw {

uint8_t FUSB302B::address = 0; // I2C address

//
// Register definitions
//
static constexpr uint8_t FUSB302B_POWER_REG_ADDR = 0x0B;

static constexpr uint8_t FUSB302B_CONTROL0_REG_ADDR = 0x06;
static constexpr uint8_t FUSB302B_CONTROL0_HOST_CUR_POS = 2;

static constexpr uint8_t FUSB302B_MASK_REG_ADDR = 0x0A;
static constexpr uint8_t FUSB302B_MASK_VBUSOK_POS = 7;
static constexpr uint8_t FUSB302B_MASK_VBUSOK_MASK = (1 << FUSB302B_MASK_VBUSOK_POS);

static constexpr uint8_t FUSB302B_MASKA_REG_ADDR = 0x0E;
static constexpr uint8_t FUSB302B_MASKB_REG_ADDR = 0x0F;

static constexpr uint8_t FUSB302B_STATUS0_REG_ADDR = 0x40;
static constexpr uint8_t FUSB302B_STATUS0_VBUSOK_POS = 7;
static constexpr uint8_t FUSB302B_STATUS0_VBUSOK_MASK = (1 << FUSB302B_STATUS0_VBUSOK_POS);

static constexpr uint8_t FUSB302B_INTERRUPT_REG_ADDR = 0x42;
static constexpr uint8_t FUSB302B_INTERRUPT_VBUSOK_POS = 7;
static constexpr uint8_t FUSB302B_INTERRUPT_VBUSOK_MASK = (1 << FUSB302B_MASK_VBUSOK_POS);

void FUSB302B::InitChip() {
    // detect and *reset* FUSB302*
    DetectAddress();

    // wake up circuit
    uint8_t _power_reg[2] = { FUSB302B_POWER_REG_ADDR, 0x0F };
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _power_reg, 2, 100);

    // mask all interrupts except VBUSOK
    uint8_t _mask_reg[2] = { FUSB302B_MASK_REG_ADDR, 0x7F & ~FUSB302B_MASK_VBUSOK_POS };
    uint8_t _maska_reg[2] = { FUSB302B_MASKA_REG_ADDR, 0x7F };
    uint8_t _maskb_reg[2] = { FUSB302B_MASKB_REG_ADDR, 0x7F };
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _mask_reg, 2, 100);
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _maska_reg, 2, 100);
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _maskb_reg, 2, 100);

    // setup default USB power
    uint8_t _control0_reg[2] = { FUSB302B_CONTROL0_REG_ADDR, 1 << FUSB302B_CONTROL0_HOST_CUR_POS };
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _control0_reg, 2, 100);
}

bool FUSB302B::ReadVBUSState() {
    uint8_t _status0_reg[2] = { FUSB302B_STATUS0_REG_ADDR, 0x0 };
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, &_status0_reg[0], 1, 100);
    (void)i2c::Receive(I2C_HANDLE_FOR(usbc), address, &_status0_reg[1], 1, 100);
    return (_status0_reg[1] & FUSB302B_STATUS0_VBUSOK_MASK);
}

void FUSB302B::ClearVBUSIntFlag() {
    // read current state
    uint8_t _interrupt_reg[2] = { FUSB302B_INTERRUPT_REG_ADDR, 0 };
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, &_interrupt_reg[0], 1, 100);
    (void)i2c::Receive(I2C_HANDLE_FOR(usbc), address, &_interrupt_reg[1], 1, 100);

    // clear I_VBUSOK and update
    _interrupt_reg[1] &= ~FUSB302B_INTERRUPT_VBUSOK_MASK;
    (void)i2c::Transmit(I2C_HANDLE_FOR(usbc), address | WRITE_FLAG, _interrupt_reg, 2, 100);
}

void FUSB302B::DetectAddress() {
    freertos::CriticalSection _;
    // FUSB302B01MPX or FUSB302BMPX is used, depending on board revision. They have different addresses, so try both and detect what chip we have.
    const uint8_t address_options[] = { 0x22 << 1, 0x23 << 1 };
    for (auto tryAddress : address_options) {
        // reset chip, if chip has this address, it will confirm the transaction
        uint8_t _sw_reset[2] = { 0x0C, 0x03 };
        auto res = i2c::Transmit(I2C_HANDLE_FOR(usbc), tryAddress | WRITE_FLAG, _sw_reset, 2, 50);
        if (res == i2c::Result::ok) {
            FUSB302B::address = tryAddress;
            return;
        }
    }
    bsod("FUSB302B address detect fail");
}
} // namespace buddy::hw
