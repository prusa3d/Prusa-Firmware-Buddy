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

#pragma once

#include "../inc/MarlinConfig.h"
#include "../lcd/ultralcd.h"

#include <option/has_puppies.h>

#if HAS_TRINAMIC

#include <TMCStepper.h>
#include "../module/planner.h"

#define TMC_X_LABEL 'X', '0'
#define TMC_Y_LABEL 'Y', '0'
#define TMC_Z_LABEL 'Z', '0'

#define TMC_X2_LABEL 'X', '2'
#define TMC_Y2_LABEL 'Y', '2'
#define TMC_Z2_LABEL 'Z', '2'
#define TMC_Z3_LABEL 'Z', '3'

#define TMC_E0_LABEL 'E', '0'
#define TMC_E1_LABEL 'E', '1'
#define TMC_E2_LABEL 'E', '2'
#define TMC_E3_LABEL 'E', '3'
#define TMC_E4_LABEL 'E', '4'
#define TMC_E5_LABEL 'E', '5'

#define CHOPPER_DEFAULT_12V  { 3, -1, 1 }
#define CHOPPER_DEFAULT_19V  { 4,  1, 1 }
#define CHOPPER_DEFAULT_24V  { 4,  2, 1 }
#define CHOPPER_DEFAULT_36V  { 5,  2, 4 }
#define CHOPPER_PRUSAMK3_24V { 3, -2, 6 }
#define CHOPPER_MARLIN_119   { 5,  2, 3 }

#if ENABLED(MONITOR_DRIVER_STATUS) && !defined(MONITOR_DRIVER_STATUS_INTERVAL_MS)
  #define MONITOR_DRIVER_STATUS_INTERVAL_MS 500u
#endif

/**
 * @brief Return feedrate (in mm/s) of the TMC period value `period`.
 * @param msteps TMC Driver configured microsteps
 * @param period The TMC step period (e.g. the TSTEP or TCOOLTHRS values) to convert
 * @param steps_per_mm Axis steps per mm
 * @return Feedrate in mm/s
 */
float tmc_period_to_feedrate(AxisEnum axis_id, uint16_t msteps, const uint32_t period, const uint32_t steps_per_mm);

/**
 * @brief Return the TMC period value for a given feedrate `feedrate`.
 * @param msteps TMC Driver configured microsteps
 * @param feedrate The TMC step period (e.g. the TSTEP or TCOOLTHRS values) to convert
 * @param steps_per_mm Axis steps per mm
 * @return TMC period value
 */
uint32_t tmc_feedrate_to_period(AxisEnum axis_id, uint16_t msteps, const float feedrate, const uint32_t steps_per_mm);

uint32_t get_homing_stall_threshold(AxisEnum axis_id);

class TMCStorage {
  protected:
    // Only a child class has access to constructor => Don't create on its own! "Poor man's abstract class"
    TMCStorage() {}

  public:
    uint16_t val_mA = 0;

    #if ENABLED(MONITOR_DRIVER_STATUS)
      uint8_t otpw_count = 0,
              error_count = 0;
      bool flag_otpw = false;
      inline bool getOTPW() { return flag_otpw; }
      inline void clear_otpw() { flag_otpw = 0; }
    #endif

    inline uint16_t getMilliamps() { return val_mA; }

    struct {
      #if HAS_STEALTHCHOP
        bool stealthChop_enabled = false;
      #endif
      #if ENABLED(HYBRID_THRESHOLD)
        uint8_t hybrid_thrs = 0;
      #endif
      #if USE_SENSORLESS
        int16_t homing_thrs = 0;
      #endif
    } stored;
};

template<class TMC>
class TMCMarlinBase : public TMC, public TMCStorage {
  public:
    const char axis_letter;
    const char driver_id;
    const AxisEnum axis_id;

    #if HAS_PUPPIES() && HAS_TOOLCHANGER()
    TMCMarlinBase(char axis_letter, char driver_id, AxisEnum axis_id, const TMC2130Stepper::Connection connection, const float RS)
      : TMC(connection, RS)
      , axis_letter(axis_letter), driver_id(driver_id), axis_id(axis_id)
      {}
    #endif
    TMCMarlinBase(char axis_letter, char driver_id, AxisEnum axis_id, const uint16_t cs_pin, const float RS)
      : TMC(cs_pin, RS)
      , axis_letter(axis_letter), driver_id(driver_id), axis_id(axis_id)
      {}
    TMCMarlinBase(char axis_letter, char driver_id, AxisEnum axis_id, const uint16_t cs_pin, const float RS, const uint8_t axis_chain_index)
      : TMC(cs_pin, RS, axis_chain_index)
      , axis_letter(axis_letter), driver_id(driver_id), axis_id(axis_id)
      {}
    TMCMarlinBase(char axis_letter, char driver_id, AxisEnum axis_id, const uint16_t CS, const float RS, const uint16_t pinMOSI, const uint16_t pinMISO, const uint16_t pinSCK)
      : TMC(CS, RS, pinMOSI, pinMISO, pinSCK)
      , axis_letter(axis_letter), driver_id(driver_id), axis_id(axis_id)
      {}
    TMCMarlinBase(char axis_letter, char driver_id, AxisEnum axis_id, const uint16_t CS, const float RS, const uint16_t pinMOSI, const uint16_t pinMISO, const uint16_t pinSCK, const uint8_t axis_chain_index)
      : TMC(CS, RS, pinMOSI, pinMISO, pinSCK,  axis_chain_index)
      , axis_letter(axis_letter), driver_id(driver_id), axis_id(axis_id)
      {}
    TMCMarlinBase(char axis_letter, char driver_id, AxisEnum axis_id, Stream * SerialPort, const float RS, const uint8_t addr)
      : TMC(SerialPort, RS, addr)
      , axis_letter(axis_letter), driver_id(driver_id), axis_id(axis_id)
      {}
    TMCMarlinBase(char axis_letter, char driver_id, AxisEnum axis_id, const uint16_t RX, const uint16_t TX, const float RS, const uint8_t addr, const bool)
      : TMC(RX, TX, RS, addr)
      , axis_letter(axis_letter), driver_id(driver_id), axis_id(axis_id)
      {}

    inline void printLabel() {
      SERIAL_CHAR(axis_letter);
      if (driver_id > '0') SERIAL_CHAR(driver_id);
    }

    inline uint16_t rms_current() { return TMC::rms_current(); }
    inline void rms_current(uint16_t mA) {
      this->val_mA = mA;
      TMC::rms_current(mA);
    }
    inline void rms_current(const uint16_t mA, const float mult) {
      this->val_mA = mA;
      TMC::rms_current(mA, mult);
    }

    #if HAS_STEALTHCHOP
      inline void refresh_stepping_mode() { this->en_pwm_mode(this->stored.stealthChop_enabled); }
      inline bool get_stealthChop_status() { return this->en_pwm_mode(); }
    #endif
    #if ENABLED(HYBRID_THRESHOLD)
      uint32_t get_pwm_thrs() {
        return tmc_feedrate_to_period(axis_id, this->microsteps(), this->TPWMTHRS(), planner.settings.axis_steps_per_mm[axis_id]);
      }
      void set_pwm_thrs(const uint32_t thrs) {
        TMC::TPWMTHRS(tmc_feedrate_to_period(axis_id, this->microsteps(), thrs, planner.settings.axis_steps_per_mm[axis_id]));
        #if HAS_LCD_MENU
          this->stored.hybrid_thrs = thrs;
        #endif
      }
    #endif
    #if USE_SENSORLESS
      static constexpr int8_t sgt_min = -64,
                              sgt_max =  63;

      inline int16_t stall_sensitivity() { return TMC::sgt(); }
      void stall_sensitivity(int16_t sgt_val) {
        sgt_val = (int16_t)constrain(sgt_val, sgt_min, sgt_max);
        TMC::sgt(sgt_val);
        #if HAS_LCD_MENU
          this->stored.homing_thrs = sgt_val;
        #endif
      }
      void stall_max_period(uint32_t max_period){
        max_period = (uint32_t)constrain(max_period, 0, 1048575);
        TMC2130Stepper::TCOOLTHRS(max_period);
      }

      #if ENABLED(SPI_ENDSTOPS)
        bool test_stall_status();
      #endif
    #endif

    #if HAS_LCD_MENU
      inline void refresh_stepper_current() { rms_current(this->val_mA); }

      #if ENABLED(HYBRID_THRESHOLD)
        inline void refresh_hybrid_thrs() { set_pwm_thrs(this->stored.hybrid_thrs); }
      #endif
      #if USE_SENSORLESS
        inline void refresh_homing_thrs() { stall_sensitivity(this->stored.homing_thrs); }
      #endif
    #endif
};

template <class TMC>
class TMCMarlin : public TMCMarlinBase<TMC> {
  public:
    using TMCMarlinBase<TMC>::TMCMarlinBase; // import constructors
};

#if HAS_DRIVER(TMC2208)
template<>
class TMCMarlin<TMC2208Stepper> : public TMCMarlinBase<TMC2208Stepper> {
  public:
    using TMCMarlinBase::TMCMarlinBase; // import constructors

    #if HAS_STEALTHCHOP
      inline void refresh_stepping_mode() { en_spreadCycle(!this->stored.stealthChop_enabled); }
      inline bool get_stealthChop_status() { return !this->en_spreadCycle(); }
    #endif
};
#endif

#if HAS_DRIVER(TMC2209)
template<>
class TMCMarlin<TMC2209Stepper> : public TMCMarlinBase<TMC2209Stepper> {
  public:
    using TMCMarlinBase::TMCMarlinBase; // import constructors

    uint8_t get_address() { return slave_address; }

    #if HAS_STEALTHCHOP
      inline void refresh_stepping_mode() { en_spreadCycle(!this->stored.stealthChop_enabled); }
      inline bool get_stealthChop_status() { return !this->en_spreadCycle(); }
    #endif
    #if USE_SENSORLESS
      static constexpr uint8_t sgt_min = 0,
                               sgt_max = 255;

      inline int16_t stall_sensitivity() { return TMC2209Stepper::SGTHRS(); }
      void stall_sensitivity(int16_t sgt_val) {
        sgt_val = (int16_t)constrain(sgt_val, sgt_min, sgt_max);
        TMC2209Stepper::SGTHRS(sgt_val);
        #if HAS_LCD_MENU
          this->stored.homing_thrs = sgt_val;
        #endif
      }
    #endif

    //Need for read/write TMC reg via g-code
    inline void write_reg(uint8_t reg, uint32_t val){
      TMC2208Stepper::write(reg, val);
    }

    inline uint32_t read_reg(uint8_t reg){
      return TMC2208Stepper::read(reg);
    }
};
#endif

template<typename TMC>
void tmc_print_current(TMC &st) {
  st.printLabel();
  SERIAL_ECHOLNPAIR(" driver current: ", st.getMilliamps());
}

#if ENABLED(MONITOR_DRIVER_STATUS)
  template<typename TMC>
  void tmc_report_otpw(TMC &st) {
    st.printLabel();
    SERIAL_ECHOPGM(" temperature prewarn triggered: ");
    serialprint_truefalse(st.getOTPW());
    SERIAL_EOL();
  }
  template<typename TMC>
  void tmc_clear_otpw(TMC &st) {
    st.clear_otpw();
    st.printLabel();
    SERIAL_ECHOLNPGM(" prewarn flag cleared");
  }
#endif
#if ENABLED(HYBRID_THRESHOLD)
  template<typename TMC>
  void tmc_print_pwmthrs(TMC &st) {
    st.printLabel();
    SERIAL_ECHOLNPAIR(" stealthChop max speed: ", st.get_pwm_thrs());
  }
#endif
#if USE_SENSORLESS
  template<typename TMC>
  void tmc_print_sgt(TMC &st) {
    st.printLabel();
    SERIAL_ECHOPGM(" homing sensitivity: ");
    SERIAL_PRINTLN(st.stall_sensitivity(), DEC);
  }
#endif

void monitor_tmc_driver();
void test_tmc_connection(const bool test_x, const bool test_y, const bool test_z, const bool test_e);

/** Similar to test_tmc_connection(true, true, true, true) but with more detailed error reporting. */
void initial_test_tmc_connection();

#if ENABLED(TMC_DEBUG)
  #if ENABLED(MONITOR_DRIVER_STATUS)
    void tmc_set_report_interval(const uint16_t update_interval);
  #endif
  void tmc_report_all(const bool print_x, const bool print_y, const bool print_z, const bool print_e);
  void tmc_get_registers(const bool print_x, const bool print_y, const bool print_z, const bool print_e);
#endif

/**
 * TMC2130-specific sensorless homing using stallGuard2.
 * stallGuard2 only works when in spreadCycle mode.
 * spreadCycle and stealthChop are mutually-exclusive.
 *
 * Defined here because of limitations with templates and headers.
 */
#if USE_SENSORLESS

  // Track enabled status of stealthChop and only re-enable where applicable
  struct sensorless_t { bool x, y, z, x2, y2, z2, z3; };

  #if ENABLED(IMPROVE_HOMING_RELIABILITY) && HOMING_SG_GUARD_DURATION > 0
    extern millis_t sg_guard_period;
    constexpr uint16_t default_sg_guard_duration = HOMING_SG_GUARD_DURATION;
  #endif

  bool tmc_enable_stallguard(TMCMarlin<TMC2130Stepper> &st);
  void tmc_disable_stallguard(TMCMarlin<TMC2130Stepper> &st, const bool restore_stealth);

  bool tmc_enable_stallguard(TMCMarlin<TMC2209Stepper> &st);
  void tmc_disable_stallguard(TMCMarlin<TMC2209Stepper> &st, const bool restore_stealth);

  bool tmc_enable_stallguard(TMCMarlin<TMC2660Stepper>);
  void tmc_disable_stallguard(TMCMarlin<TMC2660Stepper>, const bool);

  #if ENABLED(SPI_ENDSTOPS)
    template<class TMC>
    bool TMCMarlinBase<TMC>::test_stall_status() {
      this->switchCSpin(LOW);

      // read stallGuard flag from TMC library, will handle HW and SW SPI
      TMC2130_n::DRV_STATUS_t drv_status{0};
      drv_status.sr = this->DRV_STATUS();

      this->switchCSpin(HIGH);

      return drv_status.stallGuard;
    }
  #endif // SPI_ENDSTOPS

#endif // USE_SENSORLESS

#if TMC_HAS_SPI
  void tmc_init_cs_pins();
#endif

#endif // HAS_TRINAMIC
