/// @file tmc2130.cpp
#include "tmc2130.h"
#include "../config/config.h"
#include "../debug.h"

//! @defgroup TMC2130 current to value translation table
//! @{
//! @brief Translate current to tmc2130 vsense and IHOLD or IRUN - copied from MK3 FW repo.
//! @param cur current in mA
//! @return 0 .. 63
//! @n most significant bit is CHOPCONF vsense bit (sense resistor voltage based current scaling).
//! @n rest is to be used in IRUN or IHOLD register.
//!
//! | mA   | trinamic register | note |
//! | ---  | ---               | ---  |
//! |    0 |  0 | doesn't mean current off, lowest current is 1/32 current with vsense low range |
//! |   30 |  1 | |
//! |   40 |  2 | |
//! |   60 |  3 | |
//! |   90 |  4 | |
//! |  100 |  5 | |
//! |  120 |  6 | |
//! |  130 |  7 | |
//! |  150 |  8 | |
//! |  180 |  9 | |
//! |  190 | 10 | |
//! |  210 | 11 | |
//! |  230 | 12 | |
//! |  240 | 13 | |
//! |  250 | 13 | |
//! |  260 | 14 | |
//! |  280 | 15 | |
//! |  300 | 16 | |
//! |  320 | 17 | |
//! |  340 | 18 | |
//! |  350 | 19 | |
//! |  370 | 20 | |
//! |  390 | 21 | |
//! |  410 | 22 | |
//! |  430 | 23 | |
//! |  450 | 24 | |
//! |  460 | 25 | |
//! |  480 | 26 | |
//! |  500 | 27 | |
//! |  520 | 28 | |
//! |  525 | 29 | |
//! |  530 | 30 | |
//! |  540 | 33 | |
//! |  560 | 34 | |
//! |  580 | 35 | |
//! |  590 | 36 | |
//! |  610 | 37 | |
//! |  630 | 38 | |
//! |  640 | 39 | |
//! |  660 | 40 | |
//! |  670 | 41 | |
//! |  690 | 42 | |
//! |  710 | 43 | |
//! |  720 | 44 | |
//! |  730 | 45 | |
//! |  760 | 46 | |
//! |  770 | 47 | |
//! |  790 | 48 | |
//! |  810 | 49 | |
//! |  820 | 50 | |
//! |  840 | 51 | |
//! |  850 | 52 | |
//! |  870 | 53 | |
//! |  890 | 54 | |
//! |  900 | 55 | |
//! |  920 | 56 | |
//! |  940 | 57 | |
//! |  950 | 58 | |
//! |  970 | 59 | |
//! |  980 | 60 | |
//! | 1000 | 61 | |
//! | 1020 | 62 | |
//! | 1029 | 63 | |
//! @}

namespace hal {
namespace tmc2130 {

static constexpr uint8_t TOFF_DEFAULT = 3U, TOFF_MASK = 0xFU;

bool __attribute__((noinline)) TMC2130::Init(const MotorParams &params, const MotorCurrents &currents, MotorMode mode) {
    initialized = false;

    // sg_filter_threshold = (1 << (8 - params.mRes));
    sg_filter_threshold = 2;

    gpio::Init(params.csPin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::high));
    gpio::Init(params.sgPin, gpio::GPIO_InitTypeDef(gpio::Mode::input, gpio::Pull::up));
    gpio::Init(params.stepPin, gpio::GPIO_InitTypeDef(gpio::Mode::output, gpio::Level::low));

    /// check for compatible tmc driver (IOIN version field)
    uint32_t IOIN = ReadRegister(params, Registers::IOIN);

    // if the version is incorrect or a bit always set to 1 is suddenly 0
    // (the supposed SD_MODE pin that doesn't exist on this driver variant)
    if (((IOIN >> 24U) != 0x11) | !(IOIN & (1U << 6U)))
        return false; // @@TODO return some kind of failure

    /// clear reset_flag as we are (re)initializing
    errorFlags.reset_flag = false;

    // clear all error flags if possible ny reading GSTAT
    ReadRegister(params, Registers::GSTAT);

    /// apply chopper parameters
    //    const uint32_t chopconf = (uint32_t)(TOFF_DEFAULT & TOFF_MASK) << 0U // toff
    //        | (uint32_t)(5U & 0x07U) << 4U // hstrt
    //        | (uint32_t)(1U & 0x0FU) << 7U // hend
    //        | (uint32_t)(2U & 0x03U) << 15U // tbl
    //        | (uint32_t)(currents.vSense & 0x01U) << 17U // vsense
    //        | (uint32_t)(params.mRes & 0x0FU) << 24U // mres
    //        | (uint32_t)(1U & 0x01) << 28U // intpol (always true)
    //        | (uint32_t)(1U & 0x01) << 29U; // dedge (always true)
    // this ugly union/bit structure saves 34B over the previous implementation
    union ChopConfU {
        struct __attribute__((packed)) S {
            uint32_t toff : 4;
            uint32_t hstrt : 3;
            uint32_t hend : 4;
            uint32_t fd : 1;
            uint32_t disfdcc : 1;
            uint32_t rndtf : 1;
            uint32_t chm : 1;
            uint32_t tbl : 2;
            uint32_t vsense : 1;
            uint32_t vhighfs : 1;
            uint32_t vhighchm : 1;
            uint32_t sync : 4;
            uint32_t mres : 4;
            uint32_t intpol : 1;
            uint32_t dedge : 1;
            uint32_t diss2g : 1;
            uint32_t reserved : 1;
            constexpr S(bool vsense, uint8_t mres)
                : toff(TOFF_DEFAULT)
                , hstrt(5)
                , hend(1)
                , fd(0)
                , disfdcc(0)
                , rndtf(0)
                , chm(0)
                , tbl(2)
                , vsense(vsense)
                , vhighfs(0)
                , vhighchm(0)
                , sync(0)
                , mres(mres)
                , intpol(1)
                , dedge(1)
                , diss2g(0)
                , reserved(0) {}
        } s;
        uint32_t dw;
        constexpr ChopConfU(bool vsense, uint8_t mres)
            : s(vsense, mres) {}
    };
    static_assert(sizeof(ChopConfU::S) == 4);
    static_assert(sizeof(ChopConfU) == 4);

    WriteRegister(params, Registers::CHOPCONF, ChopConfU(currents.vSense, params.mRes).dw);

    /// apply currents
    SetCurrents(params, currents);

    /// instant powerdown ramp
    WriteRegister(params, Registers::TPOWERDOWN, 0);

    /// Stallguard parameters
    SetSGTHRS(params);
    WriteRegister(params, Registers::TCOOLTHRS, config::tmc2130_coolStepThreshold);

    /// Write stealth mode config and setup diag0 output
    constexpr uint32_t gconf = (uint32_t)(1U & 0x01U) << 2U // en_pwm_mode - always enabled since we can control it's effect with TPWMTHRS (0=only stealthchop, 0xFFFFF=only spreadcycle)
        | (uint32_t)(1U & 0x01U) << 7U; // diag0_stall - diag0 is open collector => active low with external pullups
    WriteRegister(params, Registers::GCONF, gconf);

    /// stealthChop parameters
    constexpr uint32_t pwmconf = ((uint32_t)(config::tmc2130_PWM_AMPL) << 0U)
        | ((uint32_t)(config::tmc2130_PWM_GRAD) << 8U)
        | ((uint32_t)(config::tmc2130_PWM_FREQ) << 16U)
        | ((uint32_t)(config::tmc2130_PWM_AUTOSCALE & 0x01U) << 18U)
        | ((uint32_t)(config::tmc2130_freewheel & 0x03U) << 20U); // special freewheeling mode only active in stealthchop when IHOLD=0 and CS=0 (actual current)
    WriteRegister(params, Registers::PWMCONF, pwmconf);

    /// TPWMTHRS: switching velocity between stealthChop and spreadCycle.
    /// Stallguard is also disabled if the velocity falls below this.
    /// Should be set as high as possible when homing.
    SetMode(params, mode);

    initialized = true;
    return true;
}

void TMC2130::SetMode(const MotorParams &params, MotorMode mode) {
    /// 0xFFFF0 is used as a "Normal" mode threshold since stealthchop will be used at standstill.
    WriteRegister(params, Registers::TPWMTHRS, (mode == Stealth) ? 70 : 0xFFFF0); // @@TODO should be configurable
}

void TMC2130::SetBridgeOutput(const MotorParams &params, bool bOn) {
    uint32_t chopconf = ReadRegister(params, Registers::CHOPCONF);
    chopconf &= ~((uint32_t)TOFF_MASK);
    if (bOn) {
        chopconf |= TOFF_DEFAULT;
    }
    WriteRegister(params, Registers::CHOPCONF, chopconf);
}

void TMC2130::SetCurrents(const MotorParams &params, const MotorCurrents &currents) {
    uint8_t iHold = currents.iHold;
    const uint8_t iRun = currents.iRun;

    //    uint32_t ihold_irun = (uint32_t)(iHold & 0x1F) << 0 // ihold
    //        | (uint32_t)(iRun & 0x1F) << 8 // irun
    //        | (uint32_t)(15 & 0x0F) << 16; // IHOLDDELAY
    // Rewriting the above code into a union makes it 18B shorter
    // Obviously, those bit shifts and ORs were not understood correctly by the compiler...
    // Now it looks nice:
    // 15f6: push r16
    // 15f8: push r17
    // 15fa: movw r30, r20
    // 15fc: ldd r16, Z+2
    // 15fe: ldd r18, Z+1
    // 1600: mov r17, r18
    // 1602: andi r17, 0x1F
    // 1604: cp r18, r16
    // 1606: brcs .+18      ; 0x161a
    // 1608: andi r16, 0x1F
    // 160a: ldi r18, 0x0F
    // 160c: ldi r19, 0x00
    // 160e: ldi r20, 0x10
    // 1610: call 0x1452    ; 0x1452 <hal::tmc2130::TMC2130::WriteRegister(...
    // 1614: pop r17
    // 1616: pop r16
    // 1618: ret
    // 161a: mov r16, r17
    // 161c: rjmp .-20      ; 0x160a
    union IHoldRun {
        struct S {
            uint8_t iHold;
            uint8_t iRun;
            uint16_t iHoldDelay;
            constexpr S(uint8_t ih, uint8_t ir)
                : iHold(ih)
                , iRun(ir)
                , iHoldDelay(15 & 0x0F) {}
        } s;
        uint32_t dw;
        constexpr IHoldRun(uint8_t ih, uint8_t ir)
            : s(ih, ir) {}
    };

    // also, make sure iHold never exceeds iRun at runtime
    IHoldRun ihold_irun((iHold > iRun ? iRun : iHold) & 0x1f, iRun & 0x1f);

    WriteRegister(params, Registers::IHOLD_IRUN, ihold_irun.dw);
}

void TMC2130::SetSGTHRS(const MotorParams &params) {
    union SGTHRSU {
        struct __attribute__((packed)) S {
            uint16_t zero;
            int8_t sgthrs : 7;
            uint8_t reserved : 1;
            uint8_t sfilt : 1;
            uint8_t reserved1 : 7;
            constexpr explicit S(int8_t sgthrs)
                : zero(0)
                , sgthrs(sgthrs)
                , reserved(0)
                , sfilt(0)
                , reserved1(0) {}
        } s;
        uint32_t dw;
        constexpr explicit SGTHRSU(int8_t sgthrs)
            : s(sgthrs) {}
    };
    static_assert(sizeof(SGTHRSU) == 4);
    //uint32_t tmc2130_coolConf = (((uint32_t)params.sg_thrs) << 16U);
    WriteRegister(params, Registers::COOLCONF, SGTHRSU(params.sg_thrs).dw);
}

void TMC2130::SetEnabled(const MotorParams &params, bool enabled) {
    hal::shr16::shr16.SetTMCEnabled(params.idx, enabled);
    if (this->enabled != enabled)
        ClearStallguard();
    this->enabled = enabled;
}

bool __attribute__((noinline)) TMC2130::CheckForErrors(const MotorParams &params) {
    if (!initialized)
        return false;

    uint32_t GSTAT = ReadRegister(params, Registers::GSTAT);
    uint32_t DRV_STATUS = ReadRegister(params, Registers::DRV_STATUS);
    errorFlags.reset_flag |= !!(GSTAT & (1U << 0U));
    errorFlags.uv_cp = !!(GSTAT & (1U << 2U));
    errorFlags.s2g = !!(DRV_STATUS & (3UL << 27U));
    errorFlags.otpw = !!(DRV_STATUS & (1UL << 26U));
    errorFlags.ot = !!(DRV_STATUS & (1UL << 25U));

    return GSTAT || errorFlags.reset_flag; // any bit in gstat is an error
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
