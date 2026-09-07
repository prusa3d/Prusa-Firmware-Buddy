/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * stepper/trinamic.cpp
 * Stepper driver indirection for Trinamic
 */

#include "../../inc/MarlinConfig.h"

#include "trinamic.h"
#include "bsod.h"

#include <option/has_planner.h>
#if HAS_PLANNER()
    #include "../planner.h"
    #include "../stepper.h"
#endif

#if ENABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    #include <config_store/store_instance.hpp>
#endif

#include <option/has_phase_stepping.h>
#if HAS_PHASE_STEPPING()
    #include <feature/phase_stepping/phase_stepping.hpp>
#endif

#include <HardwareSerial.h>

enum StealthIndex : uint8_t { STEALTH_AXIS_XY,
    STEALTH_AXIS_Z,
    STEALTH_AXIS_E };
#define _TMC_INIT(ST, STEALTH_INDEX) tmc_init(stepper##ST, ST##_CURRENT, ST##_MICROSTEPS, ST##_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_INDEX])

//   IC = TMC model number
//   ST = Stepper object letter
//   L  = Label characters
//   AI = Axis Enum Index
// SWHW = SW/SH UART selection
#if ENABLED(TMC_USE_SW_SPI)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define __TMC_SPI_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, ST##_CS_PIN, ST##_RSENSE, TMC_SW_MOSI, TMC_SW_MISO, TMC_SW_SCK, ST##_CHAIN_POS)
#else
    #define __TMC_SPI_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, ST##_CS_PIN, ST##_RSENSE, ST##_CHAIN_POS)
#endif

#define TMC_UART_HW_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, &ST##_HARDWARE_SERIAL, ST##_RSENSE, ST##_SLAVE_ADDRESS)
#define TMC_UART_SW_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, ST##_SERIAL_RX_PIN, ST##_SERIAL_TX_PIN, ST##_RSENSE, ST##_SLAVE_ADDRESS, ST##_SERIAL_RX_PIN > -1)

#define _TMC_SPI_DEFINE(IC, ST, AI) __TMC_SPI_DEFINE(IC, ST, TMC_##ST##_LABEL, AI)
#define TMC_SPI_DEFINE(ST, AI)      _TMC_SPI_DEFINE(ST##_DRIVER_TYPE, ST, AI##_AXIS)

#define _TMC_UART_DEFINE(SWHW, IC, ST, AI) TMC_UART_##SWHW##_DEFINE(IC, ST, TMC_##ST##_LABEL, AI)
#define TMC_UART_DEFINE(SWHW, ST, AI)      _TMC_UART_DEFINE(SWHW, ST##_DRIVER_TYPE, ST, AI##_AXIS)

#if ENABLED(DISTINCT_E_FACTORS) && E_STEPPERS > 1
    // #error dead code found by automatic analyses (see BFW-5461)
    #define TMC_SPI_DEFINE_E(AI)        TMC_SPI_DEFINE(E##AI, E##AI)
    #define TMC_UART_DEFINE_E(SWHW, AI) TMC_UART_DEFINE(SWHW, E##AI, E##AI)
#else
    #define TMC_SPI_DEFINE_E(AI)        TMC_SPI_DEFINE(E##AI, E)
    #define TMC_UART_DEFINE_E(SWHW, AI) TMC_UART_DEFINE(SWHW, E##AI, E)
#endif

// Stepper objects of TMC2130/TMC2160/TMC2660/TMC5130/TMC5160 steppers used
#if AXIS_HAS_SPI(X)
TMC_SPI_DEFINE(X, X);
#endif
#if AXIS_HAS_SPI(Y)
TMC_SPI_DEFINE(Y, Y);
#endif
#if AXIS_HAS_SPI(Z)
TMC_SPI_DEFINE(Z, Z);
#endif
#if AXIS_HAS_SPI(Z2)
// #error dead code found by automatic analyses (see BFW-5461)
TMC_SPI_DEFINE(Z2, Z);
#endif
#if AXIS_HAS_SPI(Z3)
// #error dead code found by automatic analyses (see BFW-5461)
TMC_SPI_DEFINE(Z3, Z);
#endif
#if AXIS_HAS_SPI(E0)
    #if BOARD_IS_XLBUDDY()
TMCMarlin<TMC2130Stepper> stepperE0('E', '0', E_AXIS, TMC2130Stepper::Connection::Remote, E0_RSENSE);
    #else
TMC_SPI_DEFINE_E(0);
    #endif
#endif
#if AXIS_HAS_SPI(E1)
// #error dead code found by automatic analyses (see BFW-5461)
TMC_SPI_DEFINE_E(1);
#endif
#if AXIS_HAS_SPI(E2)
// #error dead code found by automatic analyses (see BFW-5461)
TMC_SPI_DEFINE_E(2);
#endif
#if AXIS_HAS_SPI(E3)
// #error dead code found by automatic analyses (see BFW-5461)
TMC_SPI_DEFINE_E(3);
#endif
#if AXIS_HAS_SPI(E4)
// #error dead code found by automatic analyses (see BFW-5461)
TMC_SPI_DEFINE_E(4);
#endif
#if AXIS_HAS_SPI(E5)
// #error dead code found by automatic analyses (see BFW-5461)
TMC_SPI_DEFINE_E(5);
#endif

#if HAS_DRIVER(TMC2130)
void tmc_init(TMCMarlin<TMC2130Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf { 0 };
    chopconf.tbl = 1;
    chopconf.toff = chopper_timing.toff;
    chopconf.intpol = INTERPOLATE;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    #if ENABLED(SQUARE_WAVE_STEPPING)
    chopconf.dedge = true;
    #endif
    st.CHOPCONF(chopconf.sr);

    st.rms_current(mA, HOLD_MULTIPLIER[st.axis_id]);
    st.microsteps(microsteps);
    st.iholddelay(10);
    st.TPOWERDOWN(128); // ~2s until driver lowers to hold current

    st.en_pwm_mode(stealth);
    st.stored.stealthChop_enabled = stealth;
    PWMCONF_t pwmconf { 0 };
    pwmconf.pwm_freq = 0b01; // f_pwm = 2/683 f_clk

    #if ENABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    const bool motors_400_step = get_has_400step_xy_motors();
    #else
    constexpr bool motors_400_step = false;
    #endif

    if ('X' == st.axis_letter) {
        pwmconf.pwm_autoscale = false;
        // LDO 400 step (MK4)
        // 370 mA RMS/1phase at 60 mm/min and 5500 mm/min
        // LDO 200 step (MK3.9)
        // 258 mA RMS/1phase at 60 mm/min, 332 at 600, 355 at 1000, 372 at 2000, 369 at 3000, 352 at 4000,
        // 343 at 5000, 323 at 6000, 314 at 7000, 306 at 8000, 296 at 9000, 250 at 10000, 193 at 11000
        pwmconf.pwm_grad = motors_400_step ? 0x0C : 0x2E;
        pwmconf.pwm_ampl = motors_400_step ? 0x12 : 0x1F;
    } else if ('Y' == st.axis_letter) {
        pwmconf.pwm_autoscale = false;
        // LDO 400 step (MK4)
        // 470 mA RMS/1phase at 60 mm/min and 5500 mm/min LDO 400 step
        // LDO 200 step (MK3.9)
        // 255 mA RMS/1phase at 60 mm/min, 330mA at 600, 362 at 1000, 400 at 2000, 383 at 3000,
        // 386 at 4000, 369 at 5000, 355 at 6000, 345 at 7000, 345 at 8000, 322 at 9000, 250 at 10000
        pwmconf.pwm_grad = motors_400_step ? 0x0D : 0x30;
        pwmconf.pwm_ampl = motors_400_step ? 0x16 : 0x1F;
    } else if ('E' == st.axis_letter) {
        pwmconf.pwm_autoscale = false;
        // MOONS 200 step pancake
        // 272 mA RMS/1phase at 4 mm/min, 302 at 40, 350 at 100, 394 at 200, 418 at 300, 454 at 400,
        // 450 nominal 425 measured at 500mm/min+ due to hybrid threshold switching to SpreadCycle
        pwmconf.pwm_grad = 0x1F;
        pwmconf.pwm_ampl = 0x1F;
    } else {
        pwmconf.pwm_autoscale = true;
        pwmconf.pwm_grad = 15;
        pwmconf.pwm_ampl = 180;
    }
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
    st.set_pwm_thrs(thrs);
    #else
    UNUSED(thrs);
    #endif

    st.GSTAT(); // Clear GSTAT
}
#endif // TMC2130

#if HAS_DRIVER(TMC2160)
// #error dead code found by automatic analyses (see BFW-5461)
void tmc_init(TMCMarlin<TMC2160Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf { 0 };
    chopconf.tbl = 1;
    chopconf.toff = chopper_timing.toff;
    chopconf.intpol = INTERPOLATE;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    #if ENABLED(SQUARE_WAVE_STEPPING)
    // #error dead code found by automatic analyses (see BFW-5461)
    chopconf.dedge = true;
    #endif
    st.CHOPCONF(chopconf.sr);

    st.rms_current(mA, HOLD_MULTIPLIER[st.axis_id]);
    st.microsteps(microsteps);
    st.iholddelay(10);
    st.TPOWERDOWN(128); // ~2s until driver lowers to hold current

    st.en_pwm_mode(stealth);
    st.stored.stealthChop_enabled = stealth;

    TMC2160_n::PWMCONF_t pwmconf { 0 };
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
    // #error dead code found by automatic analyses (see BFW-5461)
    st.set_pwm_thrs(thrs);
    #else
    // #error dead code found by automatic analyses (see BFW-5461)
    UNUSED(thrs);
    #endif

    st.GSTAT(); // Clear GSTAT
}
#endif // TMC2160

//
// TMC2208/2209 Driver objects and inits
//
#if HAS_TMC220x
    #if AXIS_HAS_UART(X)
        #ifdef X_HARDWARE_SERIAL
TMC_UART_DEFINE(HW, X, X);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE(SW, X, X);
        #endif
    #endif
    #if AXIS_HAS_UART(Y)
        #ifdef Y_HARDWARE_SERIAL
TMC_UART_DEFINE(HW, Y, Y);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE(SW, Y, Y);
        #endif
    #endif
    #if AXIS_HAS_UART(Z)
        #ifdef Z_HARDWARE_SERIAL
TMC_UART_DEFINE(HW, Z, Z);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE(SW, Z, Z);
        #endif
    #endif
    #if AXIS_HAS_UART(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef Z2_HARDWARE_SERIAL
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE(HW, Z2, Z);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE(SW, Z2, Z);
        #endif
    #endif
    #if AXIS_HAS_UART(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef Z3_HARDWARE_SERIAL
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE(HW, Z3, Z);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE(SW, Z3, Z);
        #endif
    #endif
    #if AXIS_HAS_UART(E0)
        #ifdef E0_HARDWARE_SERIAL
TMC_UART_DEFINE_E(HW, 0);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(SW, 0);
        #endif
    #endif
    #if AXIS_HAS_UART(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E1_HARDWARE_SERIAL
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(HW, 1);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(SW, 1);
        #endif
    #endif
    #if AXIS_HAS_UART(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E2_HARDWARE_SERIAL
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(HW, 2);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(SW, 2);
        #endif
    #endif
    #if AXIS_HAS_UART(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E3_HARDWARE_SERIAL
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(HW, 3);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(SW, 3);
        #endif
    #endif
    #if AXIS_HAS_UART(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E4_HARDWARE_SERIAL
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(HW, 4);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(SW, 4);
        #endif
    #endif
    #if AXIS_HAS_UART(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E5_HARDWARE_SERIAL
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(HW, 5);
        #else
// #error dead code found by automatic analyses (see BFW-5461)
TMC_UART_DEFINE_E(SW, 5);
        #endif
    #endif

void tmc_serial_begin() {
    #if AXIS_HAS_UART(X)
        #ifdef X_HARDWARE_SERIAL
    X_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperX.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(Y)
        #ifdef Y_HARDWARE_SERIAL
    Y_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperY.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(Z)
        #ifdef Z_HARDWARE_SERIAL
    Z_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperZ.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef Z2_HARDWARE_SERIAL
    // #error dead code found by automatic analyses (see BFW-5461)
    Z2_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperZ2.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef Z3_HARDWARE_SERIAL
    // #error dead code found by automatic analyses (see BFW-5461)
    Z3_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperZ3.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(E0)
        #ifdef E0_HARDWARE_SERIAL
    E0_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE0.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E1_HARDWARE_SERIAL
    // #error dead code found by automatic analyses (see BFW-5461)
    E1_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE1.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E2_HARDWARE_SERIAL
    // #error dead code found by automatic analyses (see BFW-5461)
    E2_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE2.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E3_HARDWARE_SERIAL
    // #error dead code found by automatic analyses (see BFW-5461)
    E3_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE3.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E4_HARDWARE_SERIAL
    // #error dead code found by automatic analyses (see BFW-5461)
    E4_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE4.beginSerial(115200);
        #endif
    #endif
    #if AXIS_HAS_UART(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        #ifdef E5_HARDWARE_SERIAL
    // #error dead code found by automatic analyses (see BFW-5461)
    E5_HARDWARE_SERIAL.begin(115200);
        #else
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE5.beginSerial(115200);
        #endif
    #endif
}
#endif

#if HAS_DRIVER(TMC2208)
// #error dead code found by automatic analyses (see BFW-5461)
void tmc_init(TMCMarlin<TMC2208Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    TMC2208_n::GCONF_t gconf { 0 };
    gconf.pdn_disable = true; // Use UART
    gconf.mstep_reg_select = true; // Select microsteps with UART
    gconf.i_scale_analog = false;
    gconf.en_spreadcycle = !stealth;
    st.GCONF(gconf.sr);
    st.stored.stealthChop_enabled = stealth;

    TMC2208_n::CHOPCONF_t chopconf { 0 };
    chopconf.tbl = 0b01; // blank_time = 24
    chopconf.toff = chopper_timing.toff;
    chopconf.intpol = INTERPOLATE;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    #if ENABLED(SQUARE_WAVE_STEPPING)
    // #error dead code found by automatic analyses (see BFW-5461)
    chopconf.dedge = true;
    #endif
    st.CHOPCONF(chopconf.sr);

    st.rms_current(mA, HOLD_MULTIPLIER[st.axis_id]);
    st.microsteps(microsteps);
    st.iholddelay(10);
    st.TPOWERDOWN(128); // ~2s until driver lowers to hold current

    TMC2208_n::PWMCONF_t pwmconf { 0 };
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
    // #error dead code found by automatic analyses (see BFW-5461)
    st.set_pwm_thrs(thrs);
    #else
    // #error dead code found by automatic analyses (see BFW-5461)
    UNUSED(thrs);
    #endif

    st.GSTAT(0b111); // Clear
    delay(200);
}
#endif // TMC2208

#if HAS_DRIVER(TMC2209)
void tmc_init(TMCMarlin<TMC2209Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    TMC2208_n::GCONF_t gconf { 0 };
    gconf.pdn_disable = true; // Use UART
    gconf.mstep_reg_select = true; // Select microsteps with UART
    gconf.i_scale_analog = false;
    gconf.en_spreadcycle = !stealth;
    st.GCONF(gconf.sr);
    st.stored.stealthChop_enabled = stealth;

    TMC2208_n::CHOPCONF_t chopconf { 0 };
    chopconf.tbl = 0b01; // blank_time = 24
    chopconf.toff = chopper_timing.toff;
    chopconf.intpol = INTERPOLATE;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    #if ENABLED(SQUARE_WAVE_STEPPING)
    chopconf.dedge = true;
    #endif
    st.CHOPCONF(chopconf.sr);

    st.rms_current(mA, HOLD_MULTIPLIER[st.axis_id]);
    st.microsteps(microsteps);
    st.iholddelay(10);
    st.TPOWERDOWN(128); // ~2s until driver lowers to hold current

    TMC2208_n::PWMCONF_t pwmconf { 0 };
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
    // #error dead code found by automatic analyses (see BFW-5461)
    st.set_pwm_thrs(thrs);
    #else
    UNUSED(thrs);
    #endif

    st.GSTAT(0b111); // Clear
    delay(200);
}
#endif // TMC2209

#if HAS_DRIVER(TMC2660)
// #error dead code found by automatic analyses (see BFW-5461)
void tmc_init(TMCMarlin<TMC2660Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t, const bool) {
    st.begin();

    TMC2660_n::CHOPCONF_t chopconf { 0 };
    chopconf.tbl = 1;
    chopconf.toff = chopper_timing.toff;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    st.CHOPCONF(chopconf.sr);

    st.sdoff(0);
    st.rms_current(mA);
    st.microsteps(microsteps);
    #if ENABLED(SQUARE_WAVE_STEPPING)
    // #error dead code found by automatic analyses (see BFW-5461)
    st.dedge(true);
    #endif
    st.intpol(INTERPOLATE);
    st.diss2g(true); // Disable short to ground protection. Too many false readings?

    #if ENABLED(TMC_DEBUG)
                     // #error dead code found by automatic analyses (see BFW-5461)
    st.rdsel(0b01);
    #endif
}
#endif // TMC2660

#if HAS_DRIVER(TMC5130)
// #error dead code found by automatic analyses (see BFW-5461)
void tmc_init(TMCMarlin<TMC5130Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf { 0 };
    chopconf.tbl = 1;
    chopconf.toff = chopper_timing.toff;
    chopconf.intpol = INTERPOLATE;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    #if ENABLED(SQUARE_WAVE_STEPPING)
    // #error dead code found by automatic analyses (see BFW-5461)
    chopconf.dedge = true;
    #endif
    st.CHOPCONF(chopconf.sr);

    st.rms_current(mA, HOLD_MULTIPLIER[st.axis_id]);
    st.microsteps(microsteps);
    st.iholddelay(10);
    st.TPOWERDOWN(128); // ~2s until driver lowers to hold current

    st.en_pwm_mode(stealth);
    st.stored.stealthChop_enabled = stealth;

    PWMCONF_t pwmconf { 0 };
    pwmconf.pwm_freq = 0b01; // f_pwm = 2/683 f_clk
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_grad = 5;
    pwmconf.pwm_ampl = 180;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
    // #error dead code found by automatic analyses (see BFW-5461)
    st.set_pwm_thrs(thrs);
    #else
    // #error dead code found by automatic analyses (see BFW-5461)
    UNUSED(thrs);
    #endif

    st.GSTAT(); // Clear GSTAT
}
#endif // TMC5130

#if HAS_DRIVER(TMC5160)
// #error dead code found by automatic analyses (see BFW-5461)
void tmc_init(TMCMarlin<TMC5160Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf { 0 };
    chopconf.tbl = 1;
    chopconf.toff = chopper_timing.toff;
    chopconf.intpol = INTERPOLATE;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    #if ENABLED(SQUARE_WAVE_STEPPING)
    // #error dead code found by automatic analyses (see BFW-5461)
    chopconf.dedge = true;
    #endif
    st.CHOPCONF(chopconf.sr);

    st.rms_current(mA, HOLD_MULTIPLIER[st.axis_id]);
    st.microsteps(microsteps);
    st.iholddelay(10);
    st.TPOWERDOWN(128); // ~2s until driver lowers to hold current

    st.en_pwm_mode(stealth);
    st.stored.stealthChop_enabled = stealth;

    TMC2160_n::PWMCONF_t pwmconf { 0 };
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
    // #error dead code found by automatic analyses (see BFW-5461)
    st.set_pwm_thrs(thrs);
    #else
    // #error dead code found by automatic analyses (see BFW-5461)
    UNUSED(thrs);
    #endif
    st.GSTAT(); // Clear GSTAT
}
#endif // TMC5160

void restore_trinamic_drivers() {
#if AXIS_IS_TMC(X)
    stepperX.push();
#endif
#if AXIS_IS_TMC(Y)
    stepperY.push();
#endif
#if AXIS_IS_TMC(Z)
    stepperZ.push();
#endif
#if AXIS_IS_TMC(Z2)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperZ2.push();
#endif
#if AXIS_IS_TMC(Z3)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperZ3.push();
#endif
#if AXIS_IS_TMC(E0)
    stepperE0.push();
#endif
#if AXIS_IS_TMC(E1)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE1.push();
#endif
#if AXIS_IS_TMC(E2)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE2.push();
#endif
#if AXIS_IS_TMC(E3)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE3.push();
#endif
#if AXIS_IS_TMC(E4)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE4.push();
#endif
#if AXIS_IS_TMC(E5)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperE5.push();
#endif
}

void reset_trinamic_drivers() {
#if HAS_PHASE_STEPPING()
    // This function is once called during print boot, when the phase_stepping is not yet initialized
    // Instantiating the guard there was triggering an assert.
    std::optional<phase_stepping::EnsureDisabled> guard;
    if (phase_stepping::is_initialized()) {
        guard.emplace();
    }
#endif

    static constexpr bool stealthchop_by_axis[] = {
#if ENABLED(STEALTHCHOP_XY)
        true
#else
        false
#endif
        ,
#if ENABLED(STEALTHCHOP_Z)
        true
#else
        false
#endif
        ,
#if ENABLED(STEALTHCHOP_E)
        // #error dead code found by automatic analyses (see BFW-5461)
        true
#else
        false
#endif
    };

#if AXIS_IS_TMC(X)
    #if DISABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(X, STEALTH_AXIS_XY);
    #else
    tmc_init(stepperX, get_rms_current_ma_x(), get_microsteps_x(), X_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_XY]);
    #endif
#endif
#if AXIS_IS_TMC(Y)
    #if DISABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(Y, STEALTH_AXIS_XY);
    #else
    tmc_init(stepperY, get_rms_current_ma_y(), get_microsteps_y(), Y_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_XY]);
    #endif
#endif
#if AXIS_IS_TMC(Z)
    #if DISABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(Z, STEALTH_AXIS_Z);
    #else
    tmc_init(stepperZ, get_rms_current_ma_z(), get_microsteps_z(), Z_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_Z]);
    #endif
#endif
#if AXIS_IS_TMC(Z2)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(Z2, STEALTH_AXIS_Z);
#endif
#if AXIS_IS_TMC(Z3)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(Z3, STEALTH_AXIS_Z);
#endif
#if AXIS_IS_TMC(E0)
    #if DISABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    _TMC_INIT(E0, STEALTH_AXIS_E);
    #else
    tmc_init(stepperE0, get_rms_current_ma_e(), get_microsteps_e(), E0_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_E]);
    #endif
#endif
#if AXIS_IS_TMC(E1)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(E1, STEALTH_AXIS_E);
#endif
#if AXIS_IS_TMC(E2)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(E2, STEALTH_AXIS_E);
#endif
#if AXIS_IS_TMC(E3)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(E3, STEALTH_AXIS_E);
#endif
#if AXIS_IS_TMC(E4)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(E4, STEALTH_AXIS_E);
#endif
#if AXIS_IS_TMC(E5)
    // #error dead code found by automatic analyses (see BFW-5461)
    _TMC_INIT(E5, STEALTH_AXIS_E);
#endif

#if USE_SENSORLESS
    #if X_SENSORLESS
        #if AXIS_HAS_STALLGUARD(X)
    stepperX.stall_sensitivity(X_STALL_SENSITIVITY);
        #endif
    #endif
    #if Y_SENSORLESS
        #if AXIS_HAS_STALLGUARD(Y)
    stepperY.stall_sensitivity(Y_STALL_SENSITIVITY);
        #endif
    #endif
    #if Z_SENSORLESS
        #if AXIS_HAS_STALLGUARD(Z)
    stepperZ.stall_sensitivity(Z_STALL_SENSITIVITY);
        #endif
        #if AXIS_HAS_STALLGUARD(Z2)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperZ2.stall_sensitivity(Z_STALL_SENSITIVITY);
        #endif
        #if AXIS_HAS_STALLGUARD(Z3)
    // #error dead code found by automatic analyses (see BFW-5461)
    stepperZ3.stall_sensitivity(Z_STALL_SENSITIVITY);
        #endif
    #endif
#endif

#ifdef TMC_ADV
    TMC_ADV()
#endif

#ifdef HAS_TMC_WAVETABLE
    config_store().tmc_wavetable_enabled.get() ? tmc_enable_wavetable(true, true, true) : tmc_disable_wavetable(true, true, true);
#endif

#if HAS_PLANNER()
    stepper.set_directions();
#endif
}

TMCStepperType &stepper_axis(const AxisEnum axis) {
    switch (axis) {
#if AXIS_IS_TMC(X)
    case X_AXIS:
        return stepperX;
#endif
#if AXIS_IS_TMC(Y)
    case Y_AXIS:
        return stepperY;
#endif
#if AXIS_IS_TMC(Z)
    case Z_AXIS:
        return stepperZ;
#endif
#if AXIS_IS_TMC(Z2)
    // #error dead code found by automatic analyses (see BFW-5461)
    case Z2_AXIS:
        return stepperZ2;
#endif
#if AXIS_IS_TMC(Z3)
    // #error dead code found by automatic analyses (see BFW-5461)
    case Z3_AXIS:
        return stepperZ3;
#endif
#if AXIS_IS_TMC(E0)
    case E0_AXIS:
        return stepperE0;
#endif
#if AXIS_IS_TMC(E1)
    // #error dead code found by automatic analyses (see BFW-5461)
    case E1_AXIS:
        return stepperE1;
#endif
#if AXIS_IS_TMC(E2)
    // #error dead code found by automatic analyses (see BFW-5461)
    case E2_AXIS:
        return stepperE2;
#endif
#if AXIS_IS_TMC(E3)
    // #error dead code found by automatic analyses (see BFW-5461)
    case E3_AXIS:
        return stepperE3;
#endif
#if AXIS_IS_TMC(E4)
    // #error dead code found by automatic analyses (see BFW-5461)
    case E4_AXIS:
        return stepperE4;
#endif
#if AXIS_IS_TMC(E5)
    // #error dead code found by automatic analyses (see BFW-5461)
    case E5_AXIS:
        return stepperE5;
#endif
    default:
        bsod("invalid stepper axis");
    }
}
