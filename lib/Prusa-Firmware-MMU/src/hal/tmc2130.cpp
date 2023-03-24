/// @file tmc2130.cpp
#include "tmc2130.h"
#include "../config/config.h"

#include "../debug.h"

namespace hal {
namespace tmc2130 {

bool TMC2130::Init(const MotorParams &params, const MotorCurrents &currents, MotorMode mode) {
    sg_filter_threshold = (1 << (8 - params.mRes));

    gpio::Init(params.csPin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::high));
    gpio::Init(params.sgPin, gpio::GPIO_InitTypeDef(gpio::Mode::input, gpio::Pull::up));
    gpio::Init(params.stepPin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::low));

    ///check for compatible tmc driver (IOIN version field)
    uint32_t IOIN = ReadRegister(params, Registers::IOIN);

    // if the version is incorrect or a bit always set to 1 is suddenly 0
    // (the supposed SD_MODE pin that doesn't exist on this driver variant)
    if (((IOIN >> 24U) != 0x11) | !(IOIN & (1U << 6U)))
        return false; // @@TODO return some kind of failure

    ///clear reset_flag as we are (re)initializing
    errorFlags.reset_flag = false;

    ///apply chopper parameters
    const uint32_t chopconf = (uint32_t)(3U & 0x0FU) << 0U //toff
        | (uint32_t)(5U & 0x07U) << 4U //hstrt
        | (uint32_t)(1U & 0x0FU) << 7U //hend
        | (uint32_t)(2U & 0x03U) << 15U //tbl
        | (uint32_t)(currents.vSense & 0x01U) << 17U //vsense
        | (uint32_t)(params.mRes & 0x0FU) << 24U //mres
        | (uint32_t)(1U & 0x01) << 28U //intpol (always true)
        | (uint32_t)(1U & 0x01) << 29U; //dedge (always true)
    WriteRegister(params, Registers::CHOPCONF, chopconf);

    ///apply currents
    SetCurrents(params, currents);

    ///instant powerdown ramp
    WriteRegister(params, Registers::TPOWERDOWN, 0);

    ///Stallguard parameters
    uint32_t tmc2130_coolConf = (((uint32_t)params.sg_thrs) << 16U);
    WriteRegister(params, Registers::COOLCONF, tmc2130_coolConf);
    WriteRegister(params, Registers::TCOOLTHRS, config::tmc2130_coolStepThreshold);

    ///Write stealth mode config and setup diag0 output
    constexpr uint32_t gconf = (uint32_t)(1U & 0x01U) << 2U //en_pwm_mode - always enabled since we can control it's effect with TPWMTHRS (0=only stealthchop, 0xFFFFF=only spreadcycle)
        | (uint32_t)(1U & 0x01U) << 7U; //diag0_stall - diag0 is open collector => active low with external pullups
    WriteRegister(params, Registers::GCONF, gconf);

    ///stealthChop parameters
    constexpr uint32_t pwmconf = ((uint32_t)(config::tmc2130_PWM_AMPL) << 0U)
        | ((uint32_t)(config::tmc2130_PWM_GRAD) << 8U)
        | ((uint32_t)(config::tmc2130_PWM_FREQ) << 16U)
        | ((uint32_t)(config::tmc2130_PWM_AUTOSCALE & 0x01U) << 18U)
        | ((uint32_t)(config::tmc2130_freewheel & 0x03U) << 20U); //special freewheeling mode only active in stealthchop when IHOLD=0 and CS=0 (actual current)
    WriteRegister(params, Registers::PWMCONF, pwmconf);

    /// TPWMTHRS: switching velocity between stealthChop and spreadCycle.
    /// Stallguard is also disabled if the velocity falls below this.
    /// Should be set as high as possible when homing.
    SetMode(params, mode);
    return true;
}

void TMC2130::SetMode(const MotorParams &params, MotorMode mode) {
    ///0xFFFF0 is used as a "Normal" mode threshold since stealthchop will be used at standstill.
    WriteRegister(params, Registers::TPWMTHRS, (mode == Stealth) ? 70 : 0xFFFF0); // @@TODO should be configurable
}

void TMC2130::SetCurrents(const MotorParams &params, const MotorCurrents &currents) {
    uint32_t ihold_irun = (uint32_t)(currents.iHold & 0x1F) << 0 //ihold
        | (uint32_t)(currents.iRun & 0x1F) << 8 //irun
        | (uint32_t)(15 & 0x0F) << 16; //IHOLDDELAY
    WriteRegister(params, Registers::IHOLD_IRUN, ihold_irun);
}

void TMC2130::SetEnabled(const MotorParams &params, bool enabled) {
    hal::shr16::shr16.SetTMCEnabled(params.idx, enabled);
    if (this->enabled != enabled)
        ClearStallguard();
    this->enabled = enabled;
}

bool TMC2130::CheckForErrors(const MotorParams &params) {
    uint32_t GSTAT = ReadRegister(params, Registers::GSTAT);
    uint32_t DRV_STATUS = ReadRegister(params, Registers::DRV_STATUS);
    errorFlags.reset_flag |= GSTAT & (1U << 0U);
    errorFlags.uv_cp = GSTAT & (1U << 2U);
    errorFlags.s2g = DRV_STATUS & (3UL << 27U);
    errorFlags.otpw = DRV_STATUS & (1UL << 26U);
    errorFlags.ot = DRV_STATUS & (1UL << 25U);

    return GSTAT || errorFlags.reset_flag; //any bit in gstat is an error
}

uint32_t TMC2130::ReadRegister(const MotorParams &params, Registers reg) {
    uint8_t pData[5] = { (uint8_t)reg };
    _spi_tx_rx(params, pData);
    pData[0] = 0;
    _spi_tx_rx(params, pData);
    _handle_spi_status(params, pData[0]);
    return ((uint32_t)pData[1] << 24 | (uint32_t)pData[2] << 16 | (uint32_t)pData[3] << 8 | (uint32_t)pData[4]);
}

void TMC2130::WriteRegister(const MotorParams &params, Registers reg, uint32_t data) {
    uint8_t pData[5] = { (uint8_t)((uint8_t)(reg) | 0x80), (uint8_t)(data >> 24), (uint8_t)(data >> 16), (uint8_t)(data >> 8), (uint8_t)data };
    _spi_tx_rx(params, pData);
    _handle_spi_status(params, pData[0]);
}

void TMC2130::Isr(const MotorParams &params) {
    if (sg_filter_counter) {
        if (SampleDiag(params))
            sg_filter_counter--;
        else if (sg_filter_counter < sg_filter_threshold)
            sg_filter_counter++;
    }
}

void TMC2130::_spi_tx_rx(const MotorParams &params, uint8_t (&pData)[5]) {
    hal::gpio::WritePin(params.csPin, hal::gpio::Level::low);
    for (uint8_t i = 0; i < sizeof(pData); i++) {
        // @@TODO horrible hack to persuate the compiler, that the expression is const in terms of memory layout and meaning,
        // but we need to write into those registers
        pData[i] = hal::spi::TxRx(const_cast<hal::spi::SPI_TypeDef *>(params.spi), pData[i]);
    }
    hal::gpio::WritePin(params.csPin, hal::gpio::Level::high);
}

void TMC2130::_handle_spi_status(const MotorParams &params, uint8_t status) {
    //@@TODO
    // errorFlags.reset_flag |= status & (1 << 0);
    // errorFlags.driver_error |= status & (1 << 1);
}

} // namespace tmc2130
} // namespace hal
