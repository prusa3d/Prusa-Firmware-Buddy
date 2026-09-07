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

#include "tmc_util.h"
#include <libs/duration_t.h>
#include <module/printcounter.h>
#include <module/stepper/trinamic.h>
#include <module/motion.h>
#include <bsod.h>

#if !BOARD_IS_DWARF()
  #include <metric.h>
#endif

#include <option/has_dwarf.h>
#include <option/has_toolchanger.h>
#include <option/has_planner.h>
#if HAS_PLANNER()
  #include <module/stepper.h>
#endif

#if ENABLED(STOP_ON_ERROR)
  #include <Marlin.h>
#endif

#if ENABLED(TMC_DEBUG)
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "../libs/hex_print_routines.h"
  #if ENABLED(MONITOR_DRIVER_STATUS)
    // #error dead code found by automatic analyses (see BFW-5461)
    static uint16_t report_tmc_status_interval; // = 0
  #endif
#endif

#if !defined(STALL_THRESHOLD_TMC2130) && !BOARD_IS_DWARF()
  #include <config_store/store_instance.hpp>
#endif

#include <device/board.h>
#if BOARD_IS_XBUDDY()
  #include <hw_configuration.hpp>
#endif

static constexpr uint32_t TMC2130_INT_OSC_FREQ = 12650000;
static constexpr uint32_t TMC2130_EXT_OSC_FREQ = 16000000;

static inline uint32_t get_tmc_freq(AxisEnum axis_id) {
  switch (axis_id) {
  case X_AXIS:
  case Y_AXIS:
  case Z_AXIS:
#if BOARD_IS_XBUDDY()
    return buddy::hw::Configuration::Instance().has_trinamic_oscillators() ? TMC2130_EXT_OSC_FREQ : TMC2130_INT_OSC_FREQ;
#elif BOARD_IS_XLBUDDY()
    return TMC2130_EXT_OSC_FREQ;
#else
    return TMC2130_INT_OSC_FREQ;
#endif

  default:
    return TMC2130_INT_OSC_FREQ;
  }
}

// The conversion between period and feedrate is symmetric, use a shared
// implementation and just do a cast from float to uint32_t when needed.
static float period_feedrate_conversion(AxisEnum axis_id, uint16_t msteps, const float value, const float steps_per_mm) {
  if (std::abs(value) < 1e-6f) {
      return std::numeric_limits<float>::infinity();
  }
  if (std::abs(steps_per_mm) < 1e-6f) {
      bsod("0 steps per mm.");
  }
  msteps = std::max<uint16_t>(1, msteps); // 0 msteps is infact 1

  return get_tmc_freq(axis_id) * msteps / (256.f * value * steps_per_mm);
}

float tmc_period_to_feedrate(AxisEnum axis_id, uint16_t msteps, const uint32_t period, const float steps_per_mm) {
  return period_feedrate_conversion(axis_id, msteps, period, steps_per_mm);
}

uint32_t tmc_feedrate_to_period(AxisEnum axis_id, uint16_t msteps, const float feedrate, const float steps_per_mm) {
  return static_cast<uint32_t>(period_feedrate_conversion(axis_id, msteps, feedrate, steps_per_mm));
}

/**
 * Check for over temperature or short to ground error flags.
 * Report and log warning of overtemperature condition.
 * Reduce driver current in a persistent otpw condition.
 * Keep track of otpw counter so we don't reduce current on a single instance,
 * and so we don't repeatedly report warning before the condition is cleared.
 */
#if ENABLED(MONITOR_DRIVER_STATUS)

  struct TMC_driver_data {
    uint32_t drv_status;
    bool is_otpw:1,
         is_ot:1,
         is_s2g:1,
         is_error:1
         #if ENABLED(TMC_DEBUG)
           // #error dead code found by automatic analyses (see BFW-5461)
           , is_stall:1
           , is_stealth:1
           , is_standstill:1
          #if HAS_STALLGUARD
            // #error dead code found by automatic analyses (see BFW-5461)
           , sg_result_reasonable:1
          #endif
         #endif
      ;
    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      #if HAS_TMCX1X0 || HAS_TMC220x
        // #error dead code found by automatic analyses (see BFW-5461)
        uint8_t cs_actual;
      #endif
      #if HAS_STALLGUARD
        // #error dead code found by automatic analyses (see BFW-5461)
        uint16_t sg_result;
      #endif
    #endif
  };

  #if HAS_TMCX1X0

    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      static uint32_t get_pwm_scale(TMC2130Stepper &st) { return st.PWM_SCALE(); }
    #endif

    static TMC_driver_data get_driver_data(TMC2130Stepper &st) {
      constexpr uint8_t OT_bp = 25, OTPW_bp = 26;
      constexpr uint32_t S2G_bm = 0x18000000;
      #if ENABLED(TMC_DEBUG)
        // #error dead code found by automatic analyses (see BFW-5461)
        constexpr uint16_t SG_RESULT_bm = 0x3FF; // 0:9
        constexpr uint8_t STEALTH_bp = 14;
        constexpr uint32_t CS_ACTUAL_bm = 0x1F0000; // 16:20
        constexpr uint8_t STALL_GUARD_bp = 24;
        constexpr uint8_t STST_bp = 31;
      #endif
      TMC_driver_data data;
      const auto ds = data.drv_status = st.DRV_STATUS();
      #ifdef __AVR__
        // #error dead code found by automatic analyses (see BFW-5461)

        // 8-bit optimization saves up to 70 bytes of PROGMEM per axis
        uint8_t spart;
        #if ENABLED(TMC_DEBUG)
          // #error dead code found by automatic analyses (see BFW-5461)
          data.sg_result = ds & SG_RESULT_bm;
          spart = ds >> 8;
          data.is_stealth = TEST(spart, STEALTH_bp - 8);
          spart = ds >> 16;
          data.cs_actual = spart & (CS_ACTUAL_bm >> 16);
        #endif
        spart = ds >> 24;
        data.is_ot = TEST(spart, OT_bp - 24);
        data.is_otpw = TEST(spart, OTPW_bp - 24);
        data.is_s2g = !!(spart & (S2G_bm >> 24));
        #if ENABLED(TMC_DEBUG)
          // #error dead code found by automatic analyses (see BFW-5461)
          data.is_stall = TEST(spart, STALL_GUARD_bp - 24);
          data.is_standstill = TEST(spart, STST_bp - 24);
          data.sg_result_reasonable = !data.is_standstill; // sg_result has no reasonable meaning while standstill
        #endif

      #else // !__AVR__

        data.is_ot = TEST(ds, OT_bp);
        data.is_otpw = TEST(ds, OTPW_bp);
        data.is_s2g = !!(ds & S2G_bm);
        #if ENABLED(TMC_DEBUG)
          // #error dead code found by automatic analyses (see BFW-5461)
          constexpr uint8_t CS_ACTUAL_sb = 16;
          data.sg_result = ds & SG_RESULT_bm;
          data.is_stealth = TEST(ds, STEALTH_bp);
          data.cs_actual = (ds & CS_ACTUAL_bm) >> CS_ACTUAL_sb;
          data.is_stall = TEST(ds, STALL_GUARD_bp);
          data.is_standstill = TEST(ds, STST_bp);
          data.sg_result_reasonable = !data.is_standstill; // sg_result has no reasonable meaning while standstill
        #endif

      #endif // !__AVR__

      return data;
    }

  #endif // HAS_TMCX1X0

  #if HAS_TMC220x
    // #error dead code found by automatic analyses (see BFW-5461)

    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      static uint32_t get_pwm_scale(TMC2208Stepper &st) { return st.pwm_scale_sum(); }
    #endif

    static TMC_driver_data get_driver_data(TMC2208Stepper &st) {
      constexpr uint8_t OTPW_bp = 0, OT_bp = 1;
      constexpr uint8_t S2G_bm = 0b11110; // 2..5
      TMC_driver_data data;
      const auto ds = data.drv_status = st.DRV_STATUS();
      data.is_otpw = TEST(ds, OTPW_bp);
      data.is_ot = TEST(ds, OT_bp);
      data.is_s2g = !!(ds & S2G_bm);
      #if ENABLED(TMC_DEBUG)
        // #error dead code found by automatic analyses (see BFW-5461)
        constexpr uint32_t CS_ACTUAL_bm = 0x1F0000; // 16:20
        constexpr uint8_t STEALTH_bp = 30, STST_bp = 31;
        #ifdef __AVR__
          // #error dead code found by automatic analyses (see BFW-5461)
          // 8-bit optimization saves up to 12 bytes of PROGMEM per axis
          uint8_t spart = ds >> 16;
          data.cs_actual = spart & (CS_ACTUAL_bm >> 16);
          spart = ds >> 24;
          data.is_stealth = TEST(spart, STEALTH_bp - 24);
          data.is_standstill = TEST(spart, STST_bp - 24);
        #else
          // #error dead code found by automatic analyses (see BFW-5461)
          constexpr uint8_t CS_ACTUAL_sb = 16;
          data.cs_actual = (ds & CS_ACTUAL_bm) >> CS_ACTUAL_sb;
          data.is_stealth = TEST(ds, STEALTH_bp);
          data.is_standstill = TEST(ds, STST_bp);
        #endif
        #if HAS_STALLGUARD
          // #error dead code found by automatic analyses (see BFW-5461)
          data.sg_result_reasonable = false;
        #endif
      #endif
      return data;
    }

  #endif // TMC2208 || TMC2209

  #if HAS_DRIVER(TMC2660)
    // #error dead code found by automatic analyses (see BFW-5461)

    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      static uint32_t get_pwm_scale(TMC2660Stepper) { return 0; }
    #endif

    static TMC_driver_data get_driver_data(TMC2660Stepper &st) {
      constexpr uint8_t OT_bp = 1, OTPW_bp = 2;
      constexpr uint8_t S2G_bm = 0b11000;
      TMC_driver_data data;
      const auto ds = data.drv_status = st.DRVSTATUS();
      uint8_t spart = ds & 0xFF;
      data.is_otpw = TEST(spart, OTPW_bp);
      data.is_ot = TEST(spart, OT_bp);
      data.is_s2g = !!(ds & S2G_bm);
      #if ENABLED(TMC_DEBUG)
        // #error dead code found by automatic analyses (see BFW-5461)
        constexpr uint8_t STALL_GUARD_bp = 0;
        constexpr uint8_t STST_bp = 7, SG_RESULT_sp = 10;
        constexpr uint32_t SG_RESULT_bm = 0xFFC00; // 10:19
        data.is_stall = TEST(spart, STALL_GUARD_bp);
        data.is_standstill = TEST(spart, STST_bp);
        data.sg_result = (ds & SG_RESULT_bm) >> SG_RESULT_sp;
        data.sg_result_reasonable = true;
      #endif
      return data;
    }

  #endif // TMC2660

  #if ENABLED(STOP_ON_ERROR)
    void report_driver_error(const TMC_driver_data &data) {
      SERIAL_ECHOPGM(" driver error detected: 0x");
      SERIAL_PRINTLN(data.drv_status, HEX);
      if (data.is_ot) SERIAL_ECHOLNPGM("overtemperature");
      if (data.is_s2g) SERIAL_ECHOLNPGM("coil short circuit");
      #if ENABLED(TMC_DEBUG)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_report_all(true, true, true, true);
      #endif
      kill(PSTR("Driver error"));
    }
  #endif

  template<typename TMC>
  void report_driver_otpw(TMC &st) {
    char timestamp[14];
    duration_t elapsed = print_job_timer.duration();
    const bool has_days = (elapsed.value > 60*60*24L);
    (void)elapsed.toDigital(timestamp, has_days);
    SERIAL_EOL();
    SERIAL_ECHO(timestamp);
    SERIAL_ECHOPGM(": ");
    st.printLabel();
    SERIAL_ECHOLNPAIR(" driver overtemperature warning! (", st.getMilliamps(), "mA)");
  }

  template<typename TMC>
  void report_polled_driver_data(TMC &st, const TMC_driver_data &data) {
    const uint32_t pwm_scale = get_pwm_scale(st);
    st.printLabel();
    SERIAL_CHAR(':'); SERIAL_PRINT(pwm_scale, DEC);
    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      #if HAS_TMCX1X0 || HAS_TMC220x
        // #error dead code found by automatic analyses (see BFW-5461)
        SERIAL_CHAR('/'); SERIAL_PRINT(data.cs_actual, DEC);
      #endif
      #if HAS_STALLGUARD
        // #error dead code found by automatic analyses (see BFW-5461)
        SERIAL_CHAR('/');
        if (data.sg_result_reasonable)
          SERIAL_ECHO(data.sg_result);
        else
          SERIAL_CHAR('-');
      #endif
    #endif
    SERIAL_CHAR('|');
    if (st.error_count)       SERIAL_CHAR('E'); // Error
    if (data.is_ot)           SERIAL_CHAR('O'); // Over-temperature
    if (data.is_otpw)         SERIAL_CHAR('W'); // over-temperature pre-Warning
    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      if (data.is_stall)      SERIAL_CHAR('G'); // stallGuard
      if (data.is_stealth)    SERIAL_CHAR('T'); // stealthChop
      if (data.is_standstill) SERIAL_CHAR('I'); // standstIll
    #endif
    if (st.flag_otpw)         SERIAL_CHAR('F'); // otpw Flag
    SERIAL_CHAR('|');
    if (st.otpw_count > 0) SERIAL_PRINT(st.otpw_count, DEC);
    SERIAL_CHAR('\t');
  }

  template<typename TMC>
  void monitor_tmc_driver(TMC &st, const bool need_update_error_counters, const bool need_debug_reporting) {
    TMC_driver_data data = get_driver_data(st);
    if (data.drv_status == 0xFFFFFFFF || data.drv_status == 0x0) return;

    if (need_update_error_counters) {
      if (data.is_ot /* | data.s2ga | data.s2gb*/) st.error_count++;
      else if (st.error_count > 0) st.error_count--;

      #if ENABLED(STOP_ON_ERROR)
        if (st.error_count >= 10) {
          SERIAL_EOL();
          st.printLabel();
          report_driver_error(data);
        }
      #endif

      // Report if a warning was triggered
      if (data.is_otpw && st.otpw_count == 0)
        report_driver_otpw(st);

      #ifdef SHOW_ERROR_AFTER_OVERTEMP
      if (data.is_otpw) {
        kill("TMC", "TMC overtemp!");
      }
      #endif

      #if CURRENT_STEP_DOWN > 0
        // #error dead code found by automatic analyses (see BFW-5461)
        // Decrease current if is_otpw is true and driver is enabled and there's been more than 4 warnings
        if (data.is_otpw && st.otpw_count > 4) {
          uint16_t I_rms = st.getMilliamps();
          if (st.isEnabled() && I_rms > 100) {
            st.rms_current(I_rms - (CURRENT_STEP_DOWN));
            #if ENABLED(REPORT_CURRENT_CHANGE)
              // #error dead code found by automatic analyses (see BFW-5461)
              st.printLabel();
              SERIAL_ECHOLNPAIR(" current decreased to ", st.getMilliamps());
            #endif
          }
        }
      #endif

      if (data.is_otpw) {
        st.otpw_count++;
        st.flag_otpw = true;
      }
      else if (st.otpw_count > 0) st.otpw_count = 0;
    }

    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      if (need_debug_reporting)
        report_polled_driver_data(st, data);
    #endif
  }

  void monitor_tmc_driver() {
    static millis_t next_poll = 0;
    const millis_t ms = millis();
    bool need_update_error_counters = ELAPSED(ms, next_poll);
    bool need_debug_reporting = false;
    if (need_update_error_counters)
      next_poll = ms + MONITOR_DRIVER_STATUS_INTERVAL_MS;
    #if ENABLED(TMC_DEBUG)
      // #error dead code found by automatic analyses (see BFW-5461)
      static millis_t next_debug_reporting = 0;
      if (report_tmc_status_interval && ELAPSED(ms, next_debug_reporting)) {
        need_debug_reporting = true;
        next_debug_reporting = ms + report_tmc_status_interval;
      }
    #endif
    if (need_update_error_counters || need_debug_reporting) {
      #if AXIS_IS_TMC(X)
        monitor_tmc_driver(stepperX, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(Y)
        monitor_tmc_driver(stepperY, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(Z)
        monitor_tmc_driver(stepperZ, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        monitor_tmc_driver(stepperZ2, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        monitor_tmc_driver(stepperZ3, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(E0)
        monitor_tmc_driver(stepperE0, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        monitor_tmc_driver(stepperE1, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        monitor_tmc_driver(stepperE2, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        monitor_tmc_driver(stepperE3, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        monitor_tmc_driver(stepperE4, need_update_error_counters, need_debug_reporting);
      #endif
      #if AXIS_IS_TMC(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        monitor_tmc_driver(stepperE5, need_update_error_counters, need_debug_reporting);
      #endif

      #if ENABLED(TMC_DEBUG)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (need_debug_reporting) SERIAL_EOL();
      #endif
    }
  }

#endif // MONITOR_DRIVER_STATUS

#if ENABLED(TMC_DEBUG)
  // #error dead code found by automatic analyses (see BFW-5461)

  /**
   * M122 [S<0|1>] [Pnnn] Enable periodic status reports
   */
  #if ENABLED(MONITOR_DRIVER_STATUS)
    // #error dead code found by automatic analyses (see BFW-5461)
    void tmc_set_report_interval(const uint16_t update_interval) {
      if ((report_tmc_status_interval = update_interval))
        SERIAL_ECHOLNPGM("axis:pwm_scale"
          #if HAS_STEALTHCHOP
            // #error dead code found by automatic analyses (see BFW-5461)
            "/curr_scale"
          #endif
          #if HAS_STALLGUARD
            // #error dead code found by automatic analyses (see BFW-5461)
            "/mech_load"
          #endif
          "|flags|warncount"
        );
    }
  #endif

  enum TMC_debug_enum : char {
    TMC_CODES,
    TMC_UART_ADDR,
    TMC_ENABLED,
    TMC_CURRENT,
    TMC_RMS_CURRENT,
    TMC_MAX_CURRENT,
    TMC_IRUN,
    TMC_IHOLD,
    TMC_GLOBAL_SCALER,
    TMC_CS_ACTUAL,
    TMC_PWM_SCALE,
    TMC_VSENSE,
    TMC_STEALTHCHOP,
    TMC_MICROSTEPS,
    TMC_TSTEP,
    TMC_TPWMTHRS,
    TMC_TPWMTHRS_MMS,
    TMC_OTPW,
    TMC_OTPW_TRIGGERED,
    TMC_TOFF,
    TMC_TBL,
    TMC_HEND,
    TMC_HSTRT,
    TMC_SGT
  };
  enum TMC_drv_status_enum : char {
    TMC_DRV_CODES,
    TMC_STST,
    TMC_OLB,
    TMC_OLA,
    TMC_S2GB,
    TMC_S2GA,
    TMC_DRV_OTPW,
    TMC_OT,
    TMC_STALLGUARD,
    TMC_DRV_CS_ACTUAL,
    TMC_FSACTIVE,
    TMC_SG_RESULT,
    TMC_DRV_STATUS_HEX,
    TMC_T157,
    TMC_T150,
    TMC_T143,
    TMC_T120,
    TMC_STEALTH,
    TMC_S2VSB,
    TMC_S2VSA
  };
  enum TMC_get_registers_enum : char {
    TMC_AXIS_CODES,
    TMC_GET_GCONF,
    TMC_GET_IHOLD_IRUN,
    TMC_GET_GSTAT,
    TMC_GET_IOIN,
    TMC_GET_TPOWERDOWN,
    TMC_GET_TSTEP,
    TMC_GET_TPWMTHRS,
    TMC_GET_TCOOLTHRS,
    TMC_GET_THIGH,
    TMC_GET_CHOPCONF,
    TMC_GET_COOLCONF,
    TMC_GET_PWMCONF,
    TMC_GET_PWM_SCALE,
    TMC_GET_DRV_STATUS,
    TMC_GET_DRVCONF,
    TMC_GET_DRVCTRL,
    TMC_GET_DRVSTATUS,
    TMC_GET_SGCSCONF,
    TMC_GET_SMARTEN
  };

  template<class TMC>
  static void print_vsense(TMC &st) { serialprintPGM(st.vsense() ? PSTR("1=.18") : PSTR("0=.325")); }

  #if HAS_DRIVER(TMC2130) || HAS_DRIVER(TMC5130)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void _tmc_status(TMC2130Stepper &st, const TMC_debug_enum i) {
      switch (i) {
        case TMC_PWM_SCALE: SERIAL_PRINT(st.PWM_SCALE(), DEC); break;
        case TMC_SGT: SERIAL_PRINT(st.sgt(), DEC); break;
        case TMC_STEALTHCHOP: serialprint_truefalse(st.en_pwm_mode()); break;
        default: break;
      }
    }
  #endif
  #if HAS_TMCX1X0
    // #error dead code found by automatic analyses (see BFW-5461)
    static void _tmc_parse_drv_status(TMC2130Stepper &st, const TMC_drv_status_enum i) {
      switch (i) {
        case TMC_STALLGUARD: if (st.stallguard()) SERIAL_CHAR('*'); break;
        case TMC_SG_RESULT:  SERIAL_PRINT(st.sg_result(), DEC); break;
        case TMC_FSACTIVE:   if (st.fsactive())   SERIAL_CHAR('*'); break;
        case TMC_DRV_CS_ACTUAL: SERIAL_PRINT(st.cs_actual(), DEC); break;
        default: break;
      }
    }
  #endif

  #if HAS_DRIVER(TMC2160) || HAS_DRIVER(TMC5160)
    // #error dead code found by automatic analyses (see BFW-5461)
    template<char AXIS_LETTER, char DRIVER_ID, AxisEnum AXIS_ID>
    void print_vsense(TMCMarlin<TMC2160Stepper, AXIS_LETTER, DRIVER_ID, AXIS_ID> &) { }

    template<char AXIS_LETTER, char DRIVER_ID, AxisEnum AXIS_ID>
    void print_vsense(TMCMarlin<TMC5160Stepper, AXIS_LETTER, DRIVER_ID, AXIS_ID> &) { }

    static void _tmc_status(TMC2160Stepper &st, const TMC_debug_enum i) {
      switch (i) {
        case TMC_PWM_SCALE: SERIAL_PRINT(st.PWM_SCALE(), DEC); break;
        case TMC_SGT: SERIAL_PRINT(st.sgt(), DEC); break;
        case TMC_STEALTHCHOP: serialprint_truefalse(st.en_pwm_mode()); break;
        case TMC_GLOBAL_SCALER:
          {
            uint16_t value = st.GLOBAL_SCALER();
            SERIAL_PRINT(value ?: 256, DEC);
            SERIAL_ECHOPGM("/256");
          }
          break;
        default: break;
      }
    }
  #endif

  #if HAS_TMC220x
    // #error dead code found by automatic analyses (see BFW-5461)
    static void _tmc_status(TMC2208Stepper &st, const TMC_debug_enum i) {
      switch (i) {
        case TMC_PWM_SCALE: SERIAL_PRINT(st.pwm_scale_sum(), DEC); break;
        case TMC_STEALTHCHOP: serialprint_truefalse(st.stealth()); break;
        case TMC_S2VSA: if (st.s2vsa()) SERIAL_CHAR('*'); break;
        case TMC_S2VSB: if (st.s2vsb()) SERIAL_CHAR('*'); break;
        default: break;
      }
    }

    #if HAS_DRIVER(TMC2209)
      // #error dead code found by automatic analyses (see BFW-5461)
      template<char AXIS_LETTER, char DRIVER_ID, AxisEnum AXIS_ID>
      static void _tmc_status(TMCMarlin<TMC2209Stepper, AXIS_LETTER, DRIVER_ID, AXIS_ID> &st, const TMC_debug_enum i) {
        switch (i) {
          case TMC_SGT:       SERIAL_PRINT(st.SGTHRS(), DEC); break;
          case TMC_UART_ADDR: SERIAL_PRINT(st.get_address(), DEC); break;
          default:
            TMC2208Stepper *parent = &st;
            _tmc_status(*parent, i);
            break;
        }
      }
    #endif

    static void _tmc_parse_drv_status(TMC2208Stepper &st, const TMC_drv_status_enum i) {
      switch (i) {
        case TMC_T157: if (st.t157()) SERIAL_CHAR('*'); break;
        case TMC_T150: if (st.t150()) SERIAL_CHAR('*'); break;
        case TMC_T143: if (st.t143()) SERIAL_CHAR('*'); break;
        case TMC_T120: if (st.t120()) SERIAL_CHAR('*'); break;
        case TMC_DRV_CS_ACTUAL: SERIAL_PRINT(st.cs_actual(), DEC); break;
        default: break;
      }
    }
  #endif

  #if HAS_DRIVER(TMC2660)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void _tmc_parse_drv_status(TMC2660Stepper, const TMC_drv_status_enum) { }
  #endif

  template <typename TMC>
  static void tmc_status(TMC &st, const TMC_debug_enum i) {
    SERIAL_CHAR('\t');
    switch (i) {
      case TMC_CODES: st.printLabel(); break;
      case TMC_ENABLED: serialprint_truefalse(st.isEnabled()); break;
      case TMC_CURRENT: SERIAL_ECHO(st.getMilliamps()); break;
      case TMC_RMS_CURRENT: SERIAL_ECHO(st.rms_current()); break;
      case TMC_MAX_CURRENT: SERIAL_PRINT((float)st.rms_current() * 1.41, 0); break;
      case TMC_IRUN:
        SERIAL_PRINT(st.irun(), DEC);
        SERIAL_ECHOPGM("/31");
        break;
      case TMC_IHOLD:
        SERIAL_PRINT(st.ihold(), DEC);
        SERIAL_ECHOPGM("/31");
        break;
      case TMC_CS_ACTUAL:
        SERIAL_PRINT(st.cs_actual(), DEC);
        SERIAL_ECHOPGM("/31");
        break;
      case TMC_VSENSE: print_vsense(st); break;
      case TMC_MICROSTEPS: SERIAL_ECHO(st.microsteps()); break;
      case TMC_TSTEP: {
        const uint32_t tstep_value = st.TSTEP();
        if (tstep_value != 0xFFFFF) SERIAL_ECHO(tstep_value); else SERIAL_ECHOPGM("max");
      } break;
      #if ENABLED(HYBRID_THRESHOLD)
        // #error dead code found by automatic analyses (see BFW-5461)
        case TMC_TPWMTHRS: SERIAL_ECHO(uint32_t(st.TPWMTHRS())); break;
        case TMC_TPWMTHRS_MMS: {
          const uint32_t tpwmthrs_val = st.get_pwm_thrs();
          if (tpwmthrs_val) SERIAL_ECHO(tpwmthrs_val); else SERIAL_CHAR('-');
        } break;
      #endif
      case TMC_OTPW: serialprint_truefalse(st.otpw()); break;
      #if ENABLED(MONITOR_DRIVER_STATUS)
        // #error dead code found by automatic analyses (see BFW-5461)
        case TMC_OTPW_TRIGGERED: serialprint_truefalse(st.getOTPW()); break;
      #endif
      case TMC_TOFF: SERIAL_PRINT(st.toff(), DEC); break;
      case TMC_TBL: SERIAL_PRINT(st.blank_time(), DEC); break;
      case TMC_HEND: SERIAL_PRINT(st.hysteresis_end(), DEC); break;
      case TMC_HSTRT: SERIAL_PRINT(st.hysteresis_start(), DEC); break;
      default: _tmc_status(st, i); break;
    }
  }

  #if HAS_DRIVER(TMC2660)
    // #error dead code found by automatic analyses (see BFW-5461)
    template<char AXIS_LETTER, char DRIVER_ID, AxisEnum AXIS_ID>
    void tmc_status(TMCMarlin<TMC2660Stepper, AXIS_LETTER, DRIVER_ID, AXIS_ID> &st, const TMC_debug_enum i) {
      SERIAL_CHAR('\t');
      switch (i) {
        case TMC_CODES: st.printLabel(); break;
        case TMC_ENABLED: serialprint_truefalse(st.isEnabled()); break;
        case TMC_CURRENT: SERIAL_ECHO(st.getMilliamps()); break;
        case TMC_RMS_CURRENT: SERIAL_ECHO(st.rms_current()); break;
        case TMC_MAX_CURRENT: SERIAL_PRINT((float)st.rms_current() * 1.41, 0); break;
        case TMC_IRUN:
          SERIAL_PRINT(st.cs(), DEC);
          SERIAL_ECHOPGM("/31");
          break;
        case TMC_VSENSE: serialprintPGM(st.vsense() ? PSTR("1=.165") : PSTR("0=.310")); break;
        case TMC_MICROSTEPS: SERIAL_ECHO(st.microsteps()); break;
        //case TMC_OTPW: serialprint_truefalse(st.otpw()); break;
        //case TMC_OTPW_TRIGGERED: serialprint_truefalse(st.getOTPW()); break;
        case TMC_SGT: SERIAL_PRINT(st.sgt(), DEC); break;
        case TMC_TOFF: SERIAL_PRINT(st.toff(), DEC); break;
        case TMC_TBL: SERIAL_PRINT(st.blank_time(), DEC); break;
        case TMC_HEND: SERIAL_PRINT(st.hysteresis_end(), DEC); break;
        case TMC_HSTRT: SERIAL_PRINT(st.hysteresis_start(), DEC); break;
        default: break;
      }
    }
  #endif

  template <typename TMC>
  static void tmc_parse_drv_status(TMC &st, const TMC_drv_status_enum i) {
    SERIAL_CHAR('\t');
    switch (i) {
      case TMC_DRV_CODES:     st.printLabel();  break;
      case TMC_STST:          if (st.stst())         SERIAL_CHAR('*'); break;
      case TMC_OLB:           if (st.olb())          SERIAL_CHAR('*'); break;
      case TMC_OLA:           if (st.ola())          SERIAL_CHAR('*'); break;
      case TMC_S2GB:          if (st.s2gb())         SERIAL_CHAR('*'); break;
      case TMC_S2GA:          if (st.s2ga())         SERIAL_CHAR('*'); break;
      case TMC_DRV_OTPW:      if (st.otpw())         SERIAL_CHAR('*'); break;
      case TMC_OT:            if (st.ot())           SERIAL_CHAR('*'); break;
      case TMC_DRV_STATUS_HEX: {
        const uint32_t drv_status = st.DRV_STATUS();
        SERIAL_CHAR('\t');
        st.printLabel();
        SERIAL_CHAR('\t');
        print_hex_long(drv_status, ':');
        if (drv_status == 0xFFFFFFFF || drv_status == 0) SERIAL_ECHOPGM("\t Bad response!");
        SERIAL_EOL();
        break;
      }
      default: _tmc_parse_drv_status(st, i); break;
    }
  }

  static void tmc_debug_loop(const TMC_debug_enum i, const bool print_x, const bool print_y, const bool print_z, const bool print_e) {
    if (print_x) {
      #if AXIS_IS_TMC(X)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperX, i);
      #endif
    }

    if (print_y) {
      #if AXIS_IS_TMC(Y)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperY, i);
      #endif
    }

    if (print_z) {
      #if AXIS_IS_TMC(Z)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperZ, i);
      #endif
      #if AXIS_IS_TMC(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperZ2, i);
      #endif
      #if AXIS_IS_TMC(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperZ3, i);
      #endif
    }

    if (print_e) {
      #if AXIS_IS_TMC(E0)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperE0, i);
      #endif
      #if AXIS_IS_TMC(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperE1, i);
      #endif
      #if AXIS_IS_TMC(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperE2, i);
      #endif
      #if AXIS_IS_TMC(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperE3, i);
      #endif
      #if AXIS_IS_TMC(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperE4, i);
      #endif
      #if AXIS_IS_TMC(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_status(stepperE5, i);
      #endif
    }

    SERIAL_EOL();
  }

  static void drv_status_loop(const TMC_drv_status_enum i, const bool print_x, const bool print_y, const bool print_z, const bool print_e) {
    if (print_x) {
      #if AXIS_IS_TMC(X)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperX, i);
      #endif
    }

    if (print_y) {
      #if AXIS_IS_TMC(Y)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperY, i);
      #endif
    }

    if (print_z) {
      #if AXIS_IS_TMC(Z)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperZ, i);
      #endif
      #if AXIS_IS_TMC(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperZ2, i);
      #endif
      #if AXIS_IS_TMC(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperZ3, i);
      #endif
    }

    if (print_e) {
      #if AXIS_IS_TMC(E0)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperE0, i);
      #endif
      #if AXIS_IS_TMC(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperE1, i);
      #endif
      #if AXIS_IS_TMC(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperE2, i);
      #endif
      #if AXIS_IS_TMC(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperE3, i);
      #endif
      #if AXIS_IS_TMC(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperE4, i);
      #endif
      #if AXIS_IS_TMC(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_parse_drv_status(stepperE5, i);
      #endif
    }

    SERIAL_EOL();
  }

  /**
   * M122 report functions
   */

  void tmc_report_all(bool print_x, const bool print_y, const bool print_z, const bool print_e) {
    #define TMC_REPORT(LABEL, ITEM) do{ SERIAL_ECHOPGM(LABEL);  tmc_debug_loop(ITEM, print_x, print_y, print_z, print_e); }while(0)
    #define DRV_REPORT(LABEL, ITEM) do{ SERIAL_ECHOPGM(LABEL); drv_status_loop(ITEM, print_x, print_y, print_z, print_e); }while(0)
    TMC_REPORT("\t",                 TMC_CODES);
    #if HAS_DRIVER(TMC2209)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_REPORT("Address\t",        TMC_UART_ADDR);
    #endif
    TMC_REPORT("Enabled\t",          TMC_ENABLED);
    TMC_REPORT("Set current",        TMC_CURRENT);
    TMC_REPORT("RMS current",        TMC_RMS_CURRENT);
    TMC_REPORT("MAX current",        TMC_MAX_CURRENT);
    TMC_REPORT("Run current",        TMC_IRUN);
    TMC_REPORT("Hold current",       TMC_IHOLD);
    #if HAS_DRIVER(TMC2160) || HAS_DRIVER(TMC5160)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_REPORT("Global scaler",    TMC_GLOBAL_SCALER);
    #endif
    TMC_REPORT("CS actual",          TMC_CS_ACTUAL);
    TMC_REPORT("PWM scale",          TMC_PWM_SCALE);
    #if HAS_DRIVER(TMC2130) || HAS_DRIVER(TMC2224) || HAS_DRIVER(TMC2660) || HAS_TMC220x
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_REPORT("vsense\t",         TMC_VSENSE);
    #endif
    TMC_REPORT("stealthChop",        TMC_STEALTHCHOP);
    TMC_REPORT("msteps\t",           TMC_MICROSTEPS);
    TMC_REPORT("tstep\t",            TMC_TSTEP);
    TMC_REPORT("pwm\nthreshold",     TMC_TPWMTHRS);
    TMC_REPORT("[mm/s]\t",           TMC_TPWMTHRS_MMS);
    TMC_REPORT("OT prewarn",         TMC_OTPW);
    #if ENABLED(MONITOR_DRIVER_STATUS)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_REPORT("OT prewarn has\n"
                 "been triggered",   TMC_OTPW_TRIGGERED);
    #endif
    TMC_REPORT("off time",           TMC_TOFF);
    TMC_REPORT("blank time",         TMC_TBL);
    TMC_REPORT("hysteresis\n-end\t", TMC_HEND);
    TMC_REPORT("-start\t",           TMC_HSTRT);
    TMC_REPORT("Stallguard thrs",    TMC_SGT);

    DRV_REPORT("DRVSTATUS",          TMC_DRV_CODES);
    #if HAS_TMCX1X0
      // #error dead code found by automatic analyses (see BFW-5461)
      DRV_REPORT("stallguard\t",     TMC_STALLGUARD);
      DRV_REPORT("sg_result",        TMC_SG_RESULT);
      DRV_REPORT("fsactive\t",       TMC_FSACTIVE);
    #endif
    DRV_REPORT("stst\t",             TMC_STST);
    DRV_REPORT("olb\t",              TMC_OLB);
    DRV_REPORT("ola\t",              TMC_OLA);
    DRV_REPORT("s2gb\t",             TMC_S2GB);
    DRV_REPORT("s2ga\t",             TMC_S2GA);
    DRV_REPORT("otpw\t",             TMC_DRV_OTPW);
    DRV_REPORT("ot\t",               TMC_OT);
    #if HAS_TMC220x
      // #error dead code found by automatic analyses (see BFW-5461)
      DRV_REPORT("157C\t",           TMC_T157);
      DRV_REPORT("150C\t",           TMC_T150);
      DRV_REPORT("143C\t",           TMC_T143);
      DRV_REPORT("120C\t",           TMC_T120);
      DRV_REPORT("s2vsa\t",          TMC_S2VSA);
      DRV_REPORT("s2vsb\t",          TMC_S2VSB);
    #endif
    DRV_REPORT("Driver registers:\n",TMC_DRV_STATUS_HEX);
    SERIAL_EOL();
  }

  #define PRINT_TMC_REGISTER(REG_CASE) case TMC_GET_##REG_CASE: print_hex_long(st.REG_CASE(), ':'); break

  #if HAS_TMCX1X0
    // #error dead code found by automatic analyses (see BFW-5461)
    static void tmc_get_ic_registers(TMC2130Stepper &st, const TMC_get_registers_enum i) {
      switch (i) {
        PRINT_TMC_REGISTER(TCOOLTHRS);
        PRINT_TMC_REGISTER(THIGH);
        PRINT_TMC_REGISTER(COOLCONF);
        default: SERIAL_CHAR('\t'); break;
      }
    }
  #endif
  #if HAS_TMC220x
    // #error dead code found by automatic analyses (see BFW-5461)
    static void tmc_get_ic_registers(TMC2208Stepper, const TMC_get_registers_enum) { SERIAL_CHAR('\t'); }
  #endif

  #if HAS_TRINAMIC
    // #error dead code found by automatic analyses (see BFW-5461)
    template<class TMC>
    static void tmc_get_registers(TMC &st, const TMC_get_registers_enum i) {
      switch (i) {
        case TMC_AXIS_CODES: SERIAL_CHAR('\t'); st.printLabel(); break;
        PRINT_TMC_REGISTER(GCONF);
        PRINT_TMC_REGISTER(IHOLD_IRUN);
        PRINT_TMC_REGISTER(GSTAT);
        PRINT_TMC_REGISTER(IOIN);
        PRINT_TMC_REGISTER(TPOWERDOWN);
        PRINT_TMC_REGISTER(TSTEP);
        PRINT_TMC_REGISTER(TPWMTHRS);
        PRINT_TMC_REGISTER(CHOPCONF);
        PRINT_TMC_REGISTER(PWMCONF);
        PRINT_TMC_REGISTER(PWM_SCALE);
        PRINT_TMC_REGISTER(DRV_STATUS);
        default: tmc_get_ic_registers(st, i); break;
      }
      SERIAL_CHAR('\t');
    }
  #endif
  #if HAS_DRIVER(TMC2660)
    // #error dead code found by automatic analyses (see BFW-5461)
    template <char AXIS_LETTER, char DRIVER_ID, AxisEnum AXIS_ID>
    static void tmc_get_registers(TMCMarlin<TMC2660Stepper, AXIS_LETTER, DRIVER_ID, AXIS_ID> &st, const TMC_get_registers_enum i) {
      switch (i) {
        case TMC_AXIS_CODES: SERIAL_CHAR('\t'); st.printLabel(); break;
        PRINT_TMC_REGISTER(DRVCONF);
        PRINT_TMC_REGISTER(DRVCTRL);
        PRINT_TMC_REGISTER(CHOPCONF);
        PRINT_TMC_REGISTER(DRVSTATUS);
        PRINT_TMC_REGISTER(SGCSCONF);
        PRINT_TMC_REGISTER(SMARTEN);
        default: SERIAL_CHAR('\t'); break;
      }
      SERIAL_CHAR('\t');
    }
  #endif

  static void tmc_get_registers(TMC_get_registers_enum i, const bool print_x, const bool print_y, const bool print_z, const bool print_e) {
    if (print_x) {
      #if AXIS_IS_TMC(X)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperX, i);
      #endif
    }

    if (print_y) {
      #if AXIS_IS_TMC(Y)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperY, i);
      #endif
    }

    if (print_z) {
      #if AXIS_IS_TMC(Z)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperZ, i);
      #endif
      #if AXIS_IS_TMC(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperZ2, i);
      #endif
      #if AXIS_IS_TMC(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperZ3, i);
      #endif
    }

    if (print_e) {
      #if AXIS_IS_TMC(E0)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperE0, i);
      #endif
      #if AXIS_IS_TMC(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperE1, i);
      #endif
      #if AXIS_IS_TMC(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperE2, i);
      #endif
      #if AXIS_IS_TMC(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperE3, i);
      #endif
      #if AXIS_IS_TMC(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperE4, i);
      #endif
      #if AXIS_IS_TMC(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        tmc_get_registers(stepperE5, i);
      #endif
    }

    SERIAL_EOL();
  }

  void tmc_get_registers(bool print_x, bool print_y, bool print_z, bool print_e) {
    #define _TMC_GET_REG(LABEL, ITEM) do{ SERIAL_ECHOPGM(LABEL); tmc_get_registers(ITEM, print_x, print_y, print_z, print_e); }while(0)
    #define TMC_GET_REG(NAME, TABS) _TMC_GET_REG(STRINGIFY(NAME) TABS, TMC_GET_##NAME)
    _TMC_GET_REG("\t", TMC_AXIS_CODES);
    TMC_GET_REG(GCONF, "\t\t");
    TMC_GET_REG(IHOLD_IRUN, "\t");
    TMC_GET_REG(GSTAT, "\t\t");
    TMC_GET_REG(IOIN, "\t\t");
    TMC_GET_REG(TPOWERDOWN, "\t");
    TMC_GET_REG(TSTEP, "\t\t");
    TMC_GET_REG(TPWMTHRS, "\t");
    TMC_GET_REG(TCOOLTHRS, "\t");
    TMC_GET_REG(THIGH, "\t\t");
    TMC_GET_REG(CHOPCONF, "\t");
    TMC_GET_REG(COOLCONF, "\t");
    TMC_GET_REG(PWMCONF, "\t");
    TMC_GET_REG(PWM_SCALE, "\t");
    TMC_GET_REG(DRV_STATUS, "\t");
  }

#endif // TMC_DEBUG

#if USE_SENSORLESS

#if HAS_DRIVER(TMC2130)
  #if !BOARD_IS_DWARF()
  uint32_t get_homing_stall_threshold(AxisEnum axis_id) {
      switch (axis_id) {
      case X_AXIS:
      case Y_AXIS:
  #ifdef STALL_THRESHOLD_TMC2130
          return STALL_THRESHOLD_TMC2130;
  #else
          return static_cast<uint32_t>(tmc_period_to_feedrate(X_AXIS, get_microsteps_x(), static_cast<uint32_t>(HOMING_FEEDRATE_XY / 60 * 0.8f), get_steps_per_unit_x()));
  #endif
      case Z_AXIS:
          return 400;
      default:
          bsod("Wrong axis for homing stall threshold");
      }
  }
  #endif

  bool tmc_enable_stallguard(TMCMarlin<TMC2130Stepper> &st) {
    bool stealthchop_was_enabled = st.en_pwm_mode();
#ifdef STALL_THRESHOLD_TMC2130
    st.TCOOLTHRS(STALL_THRESHOLD_TMC2130);
#else
    st.TCOOLTHRS(get_homing_stall_threshold(st.axis_id));
#endif
    st.en_pwm_mode(false);
    st.diag1_stall(true);
    st.sfilt(false);

    return stealthchop_was_enabled;
  }
  void tmc_disable_stallguard(TMCMarlin<TMC2130Stepper> &st, const bool restore_stealth) {
    st.TCOOLTHRS(0);
    st.en_pwm_mode(restore_stealth);
    st.diag1_stall(false);
  }
#endif // HAS_DRIVER(TMC2130)

#if HAS_DRIVER(TMC2209)
  bool tmc_enable_stallguard(TMCMarlin<TMC2209Stepper> &st) {
    st.TCOOLTHRS(STALL_THRESHOLD_TMC2209);
    return true;
  }
  void tmc_disable_stallguard(TMCMarlin<TMC2209Stepper> &st, const bool restore_stealth _UNUSED) {
    st.TCOOLTHRS(0);
  }
#endif // HAS_DRIVER(TMC2209)

#if HAS_DRIVER(TMC2260)
  // #error dead code found by automatic analyses (see BFW-5461)
  bool tmc_enable_stallguard(TMCMarlin<TMC2660Stepper>) {
    // TODO
    return false;
  }
  void tmc_disable_stallguard(TMCMarlin<TMC2660Stepper>, const bool) {};
#endif // HAS_DRIVER(TMC2260)


#endif // USE_SENSORLESS

template<typename TMC>
static bool test_connection(TMC &st) {
  SERIAL_ECHOPGM("Testing ");
  st.printLabel();
  SERIAL_ECHOPGM(" connection... ");
  const uint8_t test_result = st.test_connection();

  if (test_result > 0) SERIAL_ECHOPGM("Error: All ");

  const char *stat;
  switch (test_result) {
    default:
    case 0: stat = PSTR("OK"); break;
    case 1: stat = PSTR("HIGH"); break;
    case 2: stat = PSTR("LOW"); break;
  }
  serialprintPGM(stat);
  SERIAL_EOL();

  return test_result;
}

void test_tmc_connection(const bool test_x, const bool test_y, const bool test_z, const bool test_e) {
  uint8_t axis_connection = 0;

  if (test_x) {
    #if AXIS_IS_TMC(X)
      axis_connection += test_connection(stepperX);
    #endif
  }

  if (test_y) {
    #if AXIS_IS_TMC(Y)
      axis_connection += test_connection(stepperY);
    #endif
  }

  if (test_z) {
    #if AXIS_IS_TMC(Z)
      axis_connection += test_connection(stepperZ);
    #endif
    #if AXIS_IS_TMC(Z2)
      // #error dead code found by automatic analyses (see BFW-5461)
      axis_connection += test_connection(stepperZ2);
    #endif
    #if AXIS_IS_TMC(Z3)
      // #error dead code found by automatic analyses (see BFW-5461)
      axis_connection += test_connection(stepperZ3);
    #endif
  }

  if (test_e) {
    #if AXIS_IS_TMC(E0)
      axis_connection += test_connection(stepperE0);
    #endif
    #if AXIS_IS_TMC(E1)
      // #error dead code found by automatic analyses (see BFW-5461)
      axis_connection += test_connection(stepperE1);
    #endif
    #if AXIS_IS_TMC(E2)
      // #error dead code found by automatic analyses (see BFW-5461)
      axis_connection += test_connection(stepperE2);
    #endif
    #if AXIS_IS_TMC(E3)
      // #error dead code found by automatic analyses (see BFW-5461)
      axis_connection += test_connection(stepperE3);
    #endif
    #if AXIS_IS_TMC(E4)
      // #error dead code found by automatic analyses (see BFW-5461)
      axis_connection += test_connection(stepperE4);
    #endif
    #if AXIS_IS_TMC(E5)
      // #error dead code found by automatic analyses (see BFW-5461)
      axis_connection += test_connection(stepperE5);
    #endif
  }

  if (axis_connection) {
	  bsod(GET_TEXT(MSG_ERROR_TMC));
  }
}

static void initial_test_tmc_connection(AxisEnum axis) {
  auto& stepper = stepper_axis(axis);
  // Do not test remotely connected stepper drivers
  // Remotely connected stepper drivers are tested individually by the dwarf/puppies
  // using this same code, but as local
  if (stepper.get_connection() == TMCStepper::Connection::Remote) {
    return;
  }

  const auto reg = stepper.DRV_STATUS();
  if(reg == 0xFFFFFFFF || reg == 0) {
    bsod("TMC error %i (0x%08lx)", (int)axis, (unsigned long)reg);
  }
}

void initial_test_tmc_connection() {
  #if AXIS_IS_TMC(X)
  initial_test_tmc_connection(X_AXIS);
  #endif
  #if AXIS_IS_TMC(Y)
  initial_test_tmc_connection(Y_AXIS);
  #endif
  #if AXIS_IS_TMC(Z)
  initial_test_tmc_connection(Z_AXIS);
  #endif
  #if AXIS_IS_TMC(E0)
  initial_test_tmc_connection(E0_AXIS);
  #endif
}

#if HAS_DRIVER(TMC2130)
static TMC2130Stepper *pStep[4] = { nullptr, nullptr, nullptr, nullptr };
#elif HAS_DRIVER(TMC2209)
static TMC2209Stepper *pStep[4] = { nullptr, nullptr, nullptr, nullptr };
#endif

constinit const tmc_reg_t tmc_reg_map[] = {
    /*  { cmd_name, reg_adr, write, read }, */
    { "gconf", 0x00, true, true },
    { "gstat", 0x01, true, true },
#if HAS_DRIVER(TMC2130)
    { "ioin", 0x04, false, true },
#endif
#if HAS_DRIVER(TMC2209)
    { "ifcnt", 0x02, false, true },
    { "slaveconf", 0x03, true, false },
    { "otp_prog", 0x04, true, false },
    { "otp_read", 0x05, false, true },
    { "ioin", 0x06, false, true },
    { "factory_conf", 0x07, true, true },
#endif
    { "ihold_irun", 0x10, true, false },
    { "tpower_down", 0x11, true, false },
    { "tstep", 0x12, false, true },
    { "tpwmthrs", 0x13, true, false },
    { "tcoolthrs", 0x14, true, false },
#if HAS_DRIVER(TMC2130)
    { "thigh", 0x15, true, false },
#endif
#if HAS_DRIVER(TMC2209)
    { "vactual", 0x22, true, false },
#endif
#if HAS_DRIVER(TMC2130)
    { "xdirect", 0x2D, true, true },
    { "vdcmin", 0x33, true, false },
#endif
#if HAS_DRIVER(TMC2209)
    { "sgthrs", 0x40, true, false },
    { "sg_result", 0x41, false, true },
    { "coolconf", 0x42, true, false },
#endif
#if HAS_DRIVER(TMC2130)
    { "mslut0", 0x60, true, false },
    { "mslut1", 0x61, true, false },
    { "mslut2", 0x62, true, false },
    { "mslut3", 0x63, true, false },
    { "mslut4", 0x64, true, false },
    { "mslut5", 0x65, true, false },
    { "mslut6", 0x66, true, false },
    { "mslut7", 0x67, true, false },
    { "mslutsel", 0x68, true, false },
    { "mslutstart", 0x69, true, false },
#endif
    { "mscnt", 0x6A, false, true },
    { "mscuract", 0x6B, false, true },
    { "chopconf", 0x6C, true, true },
#if HAS_DRIVER(TMC2130)
    { "coolconf", 0x6D, true, false },
    { "dcctrl", 0x6D, true, false },
#endif
    { "drv_status", 0x6F, false, true },
    { "pwmconf", 0x70, true, true },
    { "pwm_scale", 0x71, false, true },
#if HAS_DRIVER(TMC2209)
    { "pwm_auto", 0x72, false, true },
#endif
#if HAS_DRIVER(TMC2130)
    { "encm_ctrl", 0x72, true, false },
    { "lost_steps", 0x73, false, true },
#endif
    { NULL, 0x00, false, false },
};

#ifdef HAS_TMC_WAVETABLE
void tmc_enable_wavetable([[maybe_unused]] bool X, [[maybe_unused]] bool Y, [[maybe_unused]] bool Z) {
    if (Y) {
        pStep[Y_AXIS]->write(0x69, 0x00f80000);
        pStep[Y_AXIS]->write(0x60, 0x56ad6b6a);
        pStep[Y_AXIS]->write(0x61, 0x54aaaaab);
        pStep[Y_AXIS]->write(0x62, 0x44449252);
        pStep[Y_AXIS]->write(0x63, 0x00020208);
        pStep[Y_AXIS]->write(0x64, 0xefe00000);
        pStep[Y_AXIS]->write(0x65, 0xadbb777b);
        pStep[Y_AXIS]->write(0x66, 0x252aaab5);
        pStep[Y_AXIS]->write(0x67, 0x00810889);
        pStep[Y_AXIS]->write(0x68, 0xff940159);
    }
    if (X) {
        pStep[X_AXIS]->write(0x69, 0x00f80000);
        pStep[X_AXIS]->write(0x60, 0x5ad6dada);
        pStep[X_AXIS]->write(0x61, 0x4a9556ab);
        pStep[X_AXIS]->write(0x62, 0x84444929);
        pStep[X_AXIS]->write(0x63, 0x00001020);
        pStep[X_AXIS]->write(0x64, 0xbefe0000);
        pStep[X_AXIS]->write(0x65, 0x5b6eeeef);
        pStep[X_AXIS]->write(0x66, 0x4a55556b);
        pStep[X_AXIS]->write(0x67, 0x00810892);
        pStep[X_AXIS]->write(0x68, 0xff900159);
    }
    if (Z) {
        pStep[Z_AXIS]->write(0x69, 0x00f80000);
        pStep[Z_AXIS]->write(0x60, 0xb77bbdf6);
        pStep[Z_AXIS]->write(0x61, 0xa5556b6d);
        pStep[Z_AXIS]->write(0x62, 0x10210924);
        pStep[Z_AXIS]->write(0x63, 0x7f800000);
        pStep[Z_AXIS]->write(0x64, 0xf77befbf);
        pStep[Z_AXIS]->write(0x65, 0xdbb76eee);
        pStep[Z_AXIS]->write(0x66, 0x55555ada);
        pStep[Z_AXIS]->write(0x67, 0x0102224a);
        pStep[Z_AXIS]->write(0x68, 0xff760159);
    }
}

void tmc_disable_wavetable([[maybe_unused]] bool X, [[maybe_unused]] bool Y, [[maybe_unused]] bool Z) {
    if (Y) {
        pStep[Y_AXIS]->write(0x69, 0x00F70000);
        pStep[Y_AXIS]->write(0x60, 0xAAAAB554);
        pStep[Y_AXIS]->write(0x61, 0x4A9554AA);
        pStep[Y_AXIS]->write(0x62, 0x24492929);
        pStep[Y_AXIS]->write(0x63, 0x10104222);
        pStep[Y_AXIS]->write(0x64, 0xFBFFFFFF);
        pStep[Y_AXIS]->write(0x65, 0xB5BB777D);
        pStep[Y_AXIS]->write(0x66, 0x49295556);
        pStep[Y_AXIS]->write(0x67, 0x00404222);
        pStep[Y_AXIS]->write(0x68, 0xFFFF8056);
    }
    if (X) {
        pStep[X_AXIS]->write(0x69, 0x00F70000);
        pStep[X_AXIS]->write(0x60, 0xAAAAB554);
        pStep[X_AXIS]->write(0x61, 0x4A9554AA);
        pStep[X_AXIS]->write(0x62, 0x24492929);
        pStep[X_AXIS]->write(0x63, 0x10104222);
        pStep[X_AXIS]->write(0x64, 0xFBFFFFFF);
        pStep[X_AXIS]->write(0x65, 0xB5BB777D);
        pStep[X_AXIS]->write(0x66, 0x49295556);
        pStep[X_AXIS]->write(0x67, 0x00404222);
        pStep[X_AXIS]->write(0x68, 0xFFFF8056);
    }
    if (Z) {
        pStep[Z_AXIS]->write(0x69, 0x00F70000);
        pStep[Z_AXIS]->write(0x60, 0xAAAAB554);
        pStep[Z_AXIS]->write(0x61, 0x4A9554AA);
        pStep[Z_AXIS]->write(0x62, 0x24492929);
        pStep[Z_AXIS]->write(0x63, 0x10104222);
        pStep[Z_AXIS]->write(0x64, 0xFBFFFFFF);
        pStep[Z_AXIS]->write(0x65, 0xB5BB777D);
        pStep[Z_AXIS]->write(0x66, 0x49295556);
        pStep[Z_AXIS]->write(0x67, 0x00404222);
        pStep[Z_AXIS]->write(0x68, 0xFFFF8056);
    }
}
#endif // HAS_TMC_WAVETABLE

void init_tmc() {
    tmc_serial_lock_init();

    // pointers to TMCStepper instances
#if AXIS_IS_TMC(X)
    pStep[X_AXIS] = &stepperX;
    pStep[X_AXIS]->TCOOLTHRS(400);
#endif
#if AXIS_IS_TMC(Y)
    pStep[Y_AXIS] = &stepperY;
    pStep[Y_AXIS]->TCOOLTHRS(400);
#endif
#if AXIS_IS_TMC(Z)
    pStep[Z_AXIS] = &stepperZ;
    pStep[Z_AXIS]->TCOOLTHRS(400);
#endif
#if AXIS_IS_TMC(E0)
    pStep[E0_AXIS] = &stepperE0;
    pStep[E_AXIS]->TCOOLTHRS(400);
#endif
#if HAS_DRIVER(TMC2209)
    pStep[X_AXIS]->SLAVECONF(0x300);
    pStep[Y_AXIS]->SLAVECONF(0x300);
    pStep[Z_AXIS]->SLAVECONF(0x300);
    pStep[E_AXIS]->SLAVECONF(0x300);
#endif
}

#if !BOARD_IS_DWARF()
static char tmc_slave_addr_to_axis_character([[maybe_unused]] uint8_t slave_addr) {
    return '?'; // TODO
}

static char should_log_register_operation(uint8_t reg_addr) {
    if (reg_addr == 0x02 /*IFCNT*/ || reg_addr == 0x41 /*SG_RESULT*/) {
        return false;
    } else {
        return true;
    }
}

static const char *tmc_reg_addr_to_name(uint8_t addr) {
    const tmc_reg_t *tmc_reg = tmc_reg_map;
    while (tmc_reg->cmd_name != NULL) {
        if (tmc_reg->reg_adr == addr) {
            return tmc_reg->cmd_name;
        }
        tmc_reg++;
    }
    return "UNKNOWN";
}

void tmc_register_write_hook(uint8_t slave_addr, uint8_t reg_addr, uint32_t val) {
    if (!should_log_register_operation(reg_addr)) {
        return;
    }
    METRIC_DEF(metric_write, "tmc_write", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
    metric_record_custom(&metric_write, ",ax=%c reg=%ui,regn=\"%s\",value=%" PRIu32 "i",
        tmc_slave_addr_to_axis_character(slave_addr), reg_addr, tmc_reg_addr_to_name(reg_addr), val);
}

void tmc_register_read_hook(uint8_t slave_addr, uint8_t reg_addr, uint32_t val) {
    if (!should_log_register_operation(reg_addr)) {
        return;
    }
    METRIC_DEF(metric_read, "tmc_read", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);
    metric_record_custom(&metric_read, ",ax=%c reg=%ui,regn=\"%s\",value=%" PRIu32 "i",
        tmc_slave_addr_to_axis_character(slave_addr), reg_addr, tmc_reg_addr_to_name(reg_addr), val);
}
#endif

bool tmc_check_coils(uint8_t axis) {
    if (!pStep[axis]) {
        return false;
    }

    const bool ola = pStep[axis]->ola();
    const bool olb = pStep[axis]->olb();
    const bool s2ga = pStep[axis]->s2ga();
    const bool s2gb = pStep[axis]->s2gb();

    // Yes, we could actually tell what is wrong. For now we are fine with boolean result.
    return !ola && !olb && !s2ga && !s2gb;
}
