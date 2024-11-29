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

#if HAS_TRINAMIC

#include "trinamic.h"
#include "../stepper.h"
#include "bsod.h"

#if !BOARD_IS_DWARF()
    #include <config_store/store_instance.hpp>
#else
    #define get_default_rms_current_ma_x() (X_CURRENT)
    #define get_default_rms_current_ma_y() (Y_CURRENT)
    #define get_default_rms_current_ma_z() (Z_CURRENT)
    #define get_default_rms_current_ma_e() (E0_CURRENT)
    #define get_microsteps_x() (X_MICROSTEPS)
    #define get_microsteps_y() (Y_MICROSTEPS)
    #define get_microsteps_z() (Z_MICROSTEPS)
    #define get_microsteps_e() (E0_MICROSTEPS)
#endif

#include <feature/phase_stepping/phase_stepping.hpp>
#include <HardwareSerial.h>


enum StealthIndex : uint8_t { STEALTH_AXIS_XY, STEALTH_AXIS_Z, STEALTH_AXIS_E };
#define _TMC_INIT(ST, STEALTH_INDEX) tmc_init(stepper##ST, ST##_CURRENT, ST##_MICROSTEPS, ST##_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_INDEX])

//   IC = TMC model number
//   ST = Stepper object letter
//   L  = Label characters
//   AI = Axis Enum Index
// SWHW = SW/SH UART selection
#if ENABLED(TMC_USE_SW_SPI)
  #define __TMC_SPI_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, ST##_CS_PIN, ST##_RSENSE, TMC_SW_MOSI, TMC_SW_MISO, TMC_SW_SCK, ST##_CHAIN_POS)
#else
  #define __TMC_SPI_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, ST##_CS_PIN, ST##_RSENSE, ST##_CHAIN_POS)
#endif

#define TMC_UART_HW_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, &ST##_HARDWARE_SERIAL, ST##_RSENSE, ST##_SLAVE_ADDRESS)
#define TMC_UART_SW_DEFINE(IC, ST, L, AI) TMCMarlin<IC##Stepper> stepper##ST(L, AI, ST##_SERIAL_RX_PIN, ST##_SERIAL_TX_PIN, ST##_RSENSE, ST##_SLAVE_ADDRESS, ST##_SERIAL_RX_PIN > -1)

#define _TMC_SPI_DEFINE(IC, ST, AI) __TMC_SPI_DEFINE(IC, ST, TMC_##ST##_LABEL, AI)
#define TMC_SPI_DEFINE(ST, AI) _TMC_SPI_DEFINE(ST##_DRIVER_TYPE, ST, AI##_AXIS)

#define _TMC_UART_DEFINE(SWHW, IC, ST, AI) TMC_UART_##SWHW##_DEFINE(IC, ST, TMC_##ST##_LABEL, AI)
#define TMC_UART_DEFINE(SWHW, ST, AI) _TMC_UART_DEFINE(SWHW, ST##_DRIVER_TYPE, ST, AI##_AXIS)

#if ENABLED(DISTINCT_E_FACTORS) && E_STEPPERS > 1
  #define TMC_SPI_DEFINE_E(AI) TMC_SPI_DEFINE(E##AI, E##AI)
  #define TMC_UART_DEFINE_E(SWHW, AI) TMC_UART_DEFINE(SWHW, E##AI, E##AI)
#else
  #define TMC_SPI_DEFINE_E(AI) TMC_SPI_DEFINE(E##AI, E)
  #define TMC_UART_DEFINE_E(SWHW, AI) TMC_UART_DEFINE(SWHW, E##AI, E)
#endif

// Stepper objects of TMC2130/TMC2160/TMC2660/TMC5130/TMC5160 steppers used
#if AXIS_HAS_SPI(X)
  TMC_SPI_DEFINE(X, X);
#endif
#if AXIS_HAS_SPI(X2)
  TMC_SPI_DEFINE(X2, X);
#endif
#if AXIS_HAS_SPI(Y)
  TMC_SPI_DEFINE(Y, Y);
#endif
#if AXIS_HAS_SPI(Y2)
  TMC_SPI_DEFINE(Y2, Y);
#endif
#if AXIS_HAS_SPI(Z)
  TMC_SPI_DEFINE(Z, Z);
#endif
#if AXIS_HAS_SPI(Z2)
  TMC_SPI_DEFINE(Z2, Z);
#endif
#if AXIS_HAS_SPI(Z3)
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
  TMC_SPI_DEFINE_E(1);
#endif
#if AXIS_HAS_SPI(E2)
  TMC_SPI_DEFINE_E(2);
#endif
#if AXIS_HAS_SPI(E3)
  TMC_SPI_DEFINE_E(3);
#endif
#if AXIS_HAS_SPI(E4)
  TMC_SPI_DEFINE_E(4);
#endif
#if AXIS_HAS_SPI(E5)
  TMC_SPI_DEFINE_E(5);
#endif

#if HAS_DRIVER(TMC2130)
  void tmc_init(TMCMarlin<TMC2130Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf{0};
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
    PWMCONF_t pwmconf{0};
    pwmconf.pwm_freq = 0b01; // f_pwm = 2/683 f_clk

#if ENABLED(USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES)
    const bool motors_400_step = get_has_400step_xy_motors();
#else
    constexpr bool motors_400_step = false;
#endif

    if ('X' == st.axis_letter)
    {
        pwmconf.pwm_autoscale = false;
        //LDO 400 step (MK4)
        //370 mA RMS/1phase at 60 mm/min and 5500 mm/min
        //LDO 200 step (MK3.9)
        //258 mA RMS/1phase at 60 mm/min, 332 at 600, 355 at 1000, 372 at 2000, 369 at 3000, 352 at 4000,
        //343 at 5000, 323 at 6000, 314 at 7000, 306 at 8000, 296 at 9000, 250 at 10000, 193 at 11000
        pwmconf.pwm_grad = motors_400_step ? 0x0C : 0x2E;
        pwmconf.pwm_ampl = motors_400_step ? 0x12 : 0x1F;
    }
    else if ('Y' == st.axis_letter)
    {
        pwmconf.pwm_autoscale = false;
        //LDO 400 step (MK4)
        //470 mA RMS/1phase at 60 mm/min and 5500 mm/min LDO 400 step
        //LDO 200 step (MK3.9)
        //255 mA RMS/1phase at 60 mm/min, 330mA at 600, 362 at 1000, 400 at 2000, 383 at 3000,
        //386 at 4000, 369 at 5000, 355 at 6000, 345 at 7000, 345 at 8000, 322 at 9000, 250 at 10000
        pwmconf.pwm_grad = motors_400_step ? 0x0D : 0x30;
        pwmconf.pwm_ampl = motors_400_step ? 0x16 : 0x1F;
    }
    else if ('E' == st.axis_letter)
    {
        pwmconf.pwm_autoscale = false;
        //MOONS 200 step pancake
        //272 mA RMS/1phase at 4 mm/min, 302 at 40, 350 at 100, 394 at 200, 418 at 300, 454 at 400,
        //450 nominal 425 measured at 500mm/min+ due to hybrid threshold switching to SpreadCycle
        pwmconf.pwm_grad = 0x1F;
        pwmconf.pwm_ampl = 0x1F;
    }
    else
    {
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
  void tmc_init(TMCMarlin<TMC2160Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf{0};
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

    TMC2160_n::PWMCONF_t pwmconf{0};
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
      st.set_pwm_thrs(thrs);
    #else
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
      TMC_UART_DEFINE(SW, X, X);
    #endif
  #endif
  #if AXIS_HAS_UART(X2)
    #ifdef X2_HARDWARE_SERIAL
      TMC_UART_DEFINE(HW, X2, X);
    #else
      TMC_UART_DEFINE(SW, X2, X);
    #endif
  #endif
  #if AXIS_HAS_UART(Y)
    #ifdef Y_HARDWARE_SERIAL
      TMC_UART_DEFINE(HW, Y, Y);
    #else
      TMC_UART_DEFINE(SW, Y, Y);
    #endif
  #endif
  #if AXIS_HAS_UART(Y2)
    #ifdef Y2_HARDWARE_SERIAL
      TMC_UART_DEFINE(HW, Y2, Y);
    #else
      TMC_UART_DEFINE(SW, Y2, Y);
    #endif
  #endif
  #if AXIS_HAS_UART(Z)
    #ifdef Z_HARDWARE_SERIAL
      TMC_UART_DEFINE(HW, Z, Z);
    #else
      TMC_UART_DEFINE(SW, Z, Z);
    #endif
  #endif
  #if AXIS_HAS_UART(Z2)
    #ifdef Z2_HARDWARE_SERIAL
      TMC_UART_DEFINE(HW, Z2, Z);
    #else
      TMC_UART_DEFINE(SW, Z2, Z);
    #endif
  #endif
  #if AXIS_HAS_UART(Z3)
    #ifdef Z3_HARDWARE_SERIAL
      TMC_UART_DEFINE(HW, Z3, Z);
    #else
      TMC_UART_DEFINE(SW, Z3, Z);
    #endif
  #endif
  #if AXIS_HAS_UART(E0)
    #ifdef E0_HARDWARE_SERIAL
      TMC_UART_DEFINE_E(HW, 0);
    #else
      TMC_UART_DEFINE_E(SW, 0);
    #endif
  #endif
  #if AXIS_HAS_UART(E1)
    #ifdef E1_HARDWARE_SERIAL
      TMC_UART_DEFINE_E(HW, 1);
    #else
      TMC_UART_DEFINE_E(SW, 1);
    #endif
  #endif
  #if AXIS_HAS_UART(E2)
    #ifdef E2_HARDWARE_SERIAL
      TMC_UART_DEFINE_E(HW, 2);
    #else
      TMC_UART_DEFINE_E(SW, 2);
    #endif
  #endif
  #if AXIS_HAS_UART(E3)
    #ifdef E3_HARDWARE_SERIAL
      TMC_UART_DEFINE_E(HW, 3);
    #else
      TMC_UART_DEFINE_E(SW, 3);
    #endif
  #endif
  #if AXIS_HAS_UART(E4)
    #ifdef E4_HARDWARE_SERIAL
      TMC_UART_DEFINE_E(HW, 4);
    #else
      TMC_UART_DEFINE_E(SW, 4);
    #endif
  #endif
  #if AXIS_HAS_UART(E5)
    #ifdef E5_HARDWARE_SERIAL
      TMC_UART_DEFINE_E(HW, 5);
    #else
      TMC_UART_DEFINE_E(SW, 5);
    #endif
  #endif

  void tmc_serial_begin() {
    #if AXIS_HAS_UART(X)
      #ifdef X_HARDWARE_SERIAL
        X_HARDWARE_SERIAL.begin(115200);
      #else
        stepperX.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(X2)
      #ifdef X2_HARDWARE_SERIAL
        X2_HARDWARE_SERIAL.begin(115200);
      #else
        stepperX2.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(Y)
      #ifdef Y_HARDWARE_SERIAL
        Y_HARDWARE_SERIAL.begin(115200);
      #else
        stepperY.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(Y2)
      #ifdef Y2_HARDWARE_SERIAL
        Y2_HARDWARE_SERIAL.begin(115200);
      #else
        stepperY2.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(Z)
      #ifdef Z_HARDWARE_SERIAL
        Z_HARDWARE_SERIAL.begin(115200);
      #else
        stepperZ.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(Z2)
      #ifdef Z2_HARDWARE_SERIAL
        Z2_HARDWARE_SERIAL.begin(115200);
      #else
        stepperZ2.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(Z3)
      #ifdef Z3_HARDWARE_SERIAL
        Z3_HARDWARE_SERIAL.begin(115200);
      #else
        stepperZ3.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(E0)
      #ifdef E0_HARDWARE_SERIAL
        E0_HARDWARE_SERIAL.begin(115200);
      #else
        stepperE0.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(E1)
      #ifdef E1_HARDWARE_SERIAL
        E1_HARDWARE_SERIAL.begin(115200);
      #else
        stepperE1.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(E2)
      #ifdef E2_HARDWARE_SERIAL
        E2_HARDWARE_SERIAL.begin(115200);
      #else
        stepperE2.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(E3)
      #ifdef E3_HARDWARE_SERIAL
        E3_HARDWARE_SERIAL.begin(115200);
      #else
        stepperE3.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(E4)
      #ifdef E4_HARDWARE_SERIAL
        E4_HARDWARE_SERIAL.begin(115200);
      #else
        stepperE4.beginSerial(115200);
      #endif
    #endif
    #if AXIS_HAS_UART(E5)
      #ifdef E5_HARDWARE_SERIAL
        E5_HARDWARE_SERIAL.begin(115200);
      #else
        stepperE5.beginSerial(115200);
      #endif
    #endif
  }
#endif

#if HAS_DRIVER(TMC2208)
  void tmc_init(TMCMarlin<TMC2208Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    TMC2208_n::GCONF_t gconf{0};
    gconf.pdn_disable = true; // Use UART
    gconf.mstep_reg_select = true; // Select microsteps with UART
    gconf.i_scale_analog = false;
    gconf.en_spreadcycle = !stealth;
    st.GCONF(gconf.sr);
    st.stored.stealthChop_enabled = stealth;

    TMC2208_n::CHOPCONF_t chopconf{0};
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

    TMC2208_n::PWMCONF_t pwmconf{0};
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
      st.set_pwm_thrs(thrs);
    #else
      UNUSED(thrs);
    #endif

    st.GSTAT(0b111); // Clear
    delay(200);
  }
#endif // TMC2208

#if HAS_DRIVER(TMC2209)
  void tmc_init(TMCMarlin<TMC2209Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    TMC2208_n::GCONF_t gconf{0};
    gconf.pdn_disable = true; // Use UART
    gconf.mstep_reg_select = true; // Select microsteps with UART
    gconf.i_scale_analog = false;
    gconf.en_spreadcycle = !stealth;
    st.GCONF(gconf.sr);
    st.stored.stealthChop_enabled = stealth;

    TMC2208_n::CHOPCONF_t chopconf{0};
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

    TMC2208_n::PWMCONF_t pwmconf{0};
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
      st.set_pwm_thrs(thrs);
    #else
      UNUSED(thrs);
    #endif

    st.GSTAT(0b111); // Clear
    delay(200);
  }
#endif // TMC2209

#if HAS_DRIVER(TMC2660)
  void tmc_init(TMCMarlin<TMC2660Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t, const bool) {
    st.begin();

    TMC2660_n::CHOPCONF_t chopconf{0};
    chopconf.tbl = 1;
    chopconf.toff = chopper_timing.toff;
    chopconf.hend = chopper_timing.hend + 3;
    chopconf.hstrt = chopper_timing.hstrt - 1;
    st.CHOPCONF(chopconf.sr);

    st.sdoff(0);
    st.rms_current(mA);
    st.microsteps(microsteps);
    #if ENABLED(SQUARE_WAVE_STEPPING)
      st.dedge(true);
    #endif
    st.intpol(INTERPOLATE);
    st.diss2g(true); // Disable short to ground protection. Too many false readings?

    #if ENABLED(TMC_DEBUG)
      st.rdsel(0b01);
    #endif
  }
#endif // TMC2660

#if HAS_DRIVER(TMC5130)
  void tmc_init(TMCMarlin<TMC5130Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf{0};
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

    PWMCONF_t pwmconf{0};
    pwmconf.pwm_freq = 0b01; // f_pwm = 2/683 f_clk
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_grad = 5;
    pwmconf.pwm_ampl = 180;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
      st.set_pwm_thrs(thrs);
    #else
      UNUSED(thrs);
    #endif

    st.GSTAT(); // Clear GSTAT
  }
#endif // TMC5130

#if HAS_DRIVER(TMC5160)
  void tmc_init(TMCMarlin<TMC5160Stepper> &st, const uint16_t mA, const uint16_t microsteps, const uint32_t thrs, const bool stealth) {
    st.begin();

    CHOPCONF_t chopconf{0};
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

    TMC2160_n::PWMCONF_t pwmconf{0};
    pwmconf.pwm_lim = 12;
    pwmconf.pwm_reg = 8;
    pwmconf.pwm_autograd = true;
    pwmconf.pwm_autoscale = true;
    pwmconf.pwm_freq = 0b01;
    pwmconf.pwm_grad = 14;
    pwmconf.pwm_ofs = 36;
    st.PWMCONF(pwmconf.sr);

    #if ENABLED(HYBRID_THRESHOLD)
      st.set_pwm_thrs(thrs);
    #else
      UNUSED(thrs);
    #endif
    st.GSTAT(); // Clear GSTAT
  }
#endif // TMC5160

void restore_trinamic_drivers() {
  #if AXIS_IS_TMC(X)
    stepperX.push();
  #endif
  #if AXIS_IS_TMC(X2)
    stepperX2.push();
  #endif
  #if AXIS_IS_TMC(Y)
    stepperY.push();
  #endif
  #if AXIS_IS_TMC(Y2)
    stepperY2.push();
  #endif
  #if AXIS_IS_TMC(Z)
    stepperZ.push();
  #endif
  #if AXIS_IS_TMC(Z2)
    stepperZ2.push();
  #endif
  #if AXIS_IS_TMC(Z3)
    stepperZ3.push();
  #endif
  #if AXIS_IS_TMC(E0)
    stepperE0.push();
  #endif
  #if AXIS_IS_TMC(E1)
    stepperE1.push();
  #endif
  #if AXIS_IS_TMC(E2)
    stepperE2.push();
  #endif
  #if AXIS_IS_TMC(E3)
    stepperE3.push();
  #endif
  #if AXIS_IS_TMC(E4)
    stepperE4.push();
  #endif
  #if AXIS_IS_TMC(E5)
    stepperE5.push();
  #endif
}

void reset_trinamic_drivers() {
  assert(!phase_stepping::any_axis_enabled());

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
      true
    #else
      false
    #endif
  };

  #if AXIS_IS_TMC(X)
tmc_init(stepperX, get_default_rms_current_ma_x(), get_microsteps_x(), X_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_XY]);
  #endif
  #if AXIS_IS_TMC(X2)
    _TMC_INIT(X2, STEALTH_AXIS_XY);
  #endif
  #if AXIS_IS_TMC(Y)
tmc_init(stepperY, get_default_rms_current_ma_y(), get_microsteps_y(), Y_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_XY]);
  #endif
  #if AXIS_IS_TMC(Y2)
    _TMC_INIT(Y2, STEALTH_AXIS_XY);
  #endif
  #if AXIS_IS_TMC(Z)
tmc_init(stepperZ, get_default_rms_current_ma_z(), get_microsteps_z(), Z_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_Z]);
  #endif
  #if AXIS_IS_TMC(Z2)
    _TMC_INIT(Z2, STEALTH_AXIS_Z);
  #endif
  #if AXIS_IS_TMC(Z3)
    _TMC_INIT(Z3, STEALTH_AXIS_Z);
  #endif
  #if AXIS_IS_TMC(E0)
tmc_init(stepperE0, get_default_rms_current_ma_e(), get_microsteps_e(), E0_HYBRID_THRESHOLD, stealthchop_by_axis[STEALTH_AXIS_E]);
  #endif
  #if AXIS_IS_TMC(E1)
    _TMC_INIT(E1, STEALTH_AXIS_E);
  #endif
  #if AXIS_IS_TMC(E2)
    _TMC_INIT(E2, STEALTH_AXIS_E);
  #endif
  #if AXIS_IS_TMC(E3)
    _TMC_INIT(E3, STEALTH_AXIS_E);
  #endif
  #if AXIS_IS_TMC(E4)
    _TMC_INIT(E4, STEALTH_AXIS_E);
  #endif
  #if AXIS_IS_TMC(E5)
    _TMC_INIT(E5, STEALTH_AXIS_E);
  #endif

  #if USE_SENSORLESS
    #if X_SENSORLESS
      #if AXIS_HAS_STALLGUARD(X)
        stepperX.stall_sensitivity(X_STALL_SENSITIVITY);
      #endif
      #if AXIS_HAS_STALLGUARD(X2) && !X2_SENSORLESS
        stepperX2.stall_sensitivity(X_STALL_SENSITIVITY);
      #endif
    #endif
    #if X2_SENSORLESS
      stepperX2.stall_sensitivity(X2_STALL_SENSITIVITY);
    #endif
    #if Y_SENSORLESS
      #if AXIS_HAS_STALLGUARD(Y)
        stepperY.stall_sensitivity(Y_STALL_SENSITIVITY);
      #endif
      #if AXIS_HAS_STALLGUARD(Y2)
        stepperY2.stall_sensitivity(Y_STALL_SENSITIVITY);
      #endif
    #endif
    #if Z_SENSORLESS
      #if AXIS_HAS_STALLGUARD(Z)
        stepperZ.stall_sensitivity(Z_STALL_SENSITIVITY);
      #endif
      #if AXIS_HAS_STALLGUARD(Z2)
        stepperZ2.stall_sensitivity(Z_STALL_SENSITIVITY);
      #endif
      #if AXIS_HAS_STALLGUARD(Z3)
        stepperZ3.stall_sensitivity(Z_STALL_SENSITIVITY);
      #endif
    #endif
  #endif

  #ifdef TMC_ADV
    TMC_ADV()
  #endif

  stepper.set_directions();
}

TMCStepper &stepper_axis(const AxisEnum axis)
{
  switch (axis) {
  #if AXIS_IS_TMC(X)
  case X_AXIS:
    return stepperX;
  #endif
  #if AXIS_IS_TMC(X2)
  case X2_AXIS:
    return stepperX2;
  #endif
  #if AXIS_IS_TMC(Y)
  case Y_AXIS:
    return stepperY;
  #endif
  #if AXIS_IS_TMC(Y2)
  case Y2_AXIS:
    return stepperY2;
  #endif
  #if AXIS_IS_TMC(Z)
  case Z_AXIS:
    return stepperZ;
  #endif
  #if AXIS_IS_TMC(Z2)
  case Z2_AXIS:
    return stepperZ2;
  #endif
  #if AXIS_IS_TMC(Z3)
  case Z3_AXIS:
    return stepperZ3;
  #endif
  #if AXIS_IS_TMC(E0)
  case E0_AXIS:
    return stepperE0;
  #endif
  #if AXIS_IS_TMC(E1)
  case E1_AXIS:
    return stepperE1;
  #endif
  #if AXIS_IS_TMC(E2)
  case E2_AXIS:
    return stepperE2;
  #endif
  #if AXIS_IS_TMC(E3)
  case E3_AXIS:
    return stepperE3;
  #endif
  #if AXIS_IS_TMC(E4)
  case E4_AXIS:
    return stepperE4;
  #endif
  #if AXIS_IS_TMC(E5)
  case E5_AXIS:
    return stepperE5;
  #endif
  default:
    bsod("invalid stepper axis");
  }
}

uint16_t stepper_microsteps(const AxisEnum axis, uint16_t new_microsteps)
{
    assert(!phase_stepping::is_enabled(axis));
    uint16_t cur_microsteps = stepper_axis(axis).microsteps();
    if (new_microsteps) {
        stepper_axis(axis).microsteps(new_microsteps);
    }
    return cur_microsteps;
}

uint16_t stepper_mscnt(const AxisEnum axis)
{
    return stepper_axis(axis).MSCNT();
}

bool stepper_wait_for_standstill(uint8_t axis_mask, millis_t max_delay) {
    millis_t timeout = millis() + max_delay;
    for (;;) {
        bool stst = true;
        LOOP_L_N(i, XYZE_N) {
            if (TEST(axis_mask, i)) {
                if (!static_cast<TMC2130Stepper &>(stepper_axis((AxisEnum)i)).stst()) {
                    stst = false;
                    break;
                }
            }
        }
        if (stst) {
          break;
        }
        if (millis() > timeout || planner.draining()) {
            return false;
        }
        safe_delay(10);
    }
    return true;
}

#endif // HAS_TRINAMIC
