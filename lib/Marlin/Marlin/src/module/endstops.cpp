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
 * endstops.cpp - A singleton object to manage endstops
 */

#include "endstops.h"
#include "stepper.h"

#include "../Marlin.h"
#include "../sd/cardreader.h"
#include "temperature.h"
#include "../lcd/ultralcd.h"
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>

#if ENABLED(ENDSTOP_INTERRUPTS_FEATURE)
  #include HAL_PATH(../HAL, endstop_interrupts.h)
#endif

#if BOTH(SD_ABORT_ON_ENDSTOP_HIT, SDSUPPORT)
  #include "printcounter.h" // for print_job_timer
#endif

#if ENABLED(BLTOUCH)
  #include "../feature/bltouch.h"
#endif

#if ENABLED(JOYSTICK)
  #include "../feature/joystick.h"
#endif

#if HAS_LOADCELL()
  #include "loadcell.hpp"
#endif

Endstops endstops;

// private:

bool Endstops::enabled, Endstops::enabled_globally; // Initialized by settings.load()
volatile uint8_t Endstops::hit_state;

Endstops::esbits_t Endstops::live_state = 0;

#if ENDSTOP_NOISE_THRESHOLD
  Endstops::esbits_t Endstops::validated_live_state;
  uint8_t Endstops::endstop_poll_count;
#endif

#if HAS_BED_PROBE
  std::atomic<bool> Endstops::z_probe_enabled { false };
#endif

#ifdef PRUSA_TOOLCHANGER
  std::atomic<bool> Endstops::xy_probe_enabled { false };
#endif

// Initialized by settings.load()
#if ENABLED(X_DUAL_ENDSTOPS)
  float Endstops::x2_endstop_adj;
#endif
#if ENABLED(Y_DUAL_ENDSTOPS)
  float Endstops::y2_endstop_adj;
#endif
#if Z_MULTI_ENDSTOPS
  float Endstops::z2_endstop_adj;
#endif
#if ENABLED(Z_TRIPLE_ENDSTOPS)
  float Endstops::z3_endstop_adj;
#endif

#if ENABLED(SPI_ENDSTOPS)
  Endstops::tmc_spi_homing_t Endstops::tmc_spi_homing; // = 0
#endif
#if ENABLED(IMPROVE_HOMING_RELIABILITY) && HOMING_SG_GUARD_DURATION > 0
  millis_t sg_guard_period; // = 0
#endif

/**
 * Class and Instance Methods
 */

void Endstops::init() {

  #if HAS_X_MIN
    #if ENABLED(ENDSTOPPULLUP_XMIN)
      SET_INPUT_PULLUP(X_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_XMIN)
      SET_INPUT_PULLDOWN(X_MIN_PIN);
    #else
      SET_INPUT(X_MIN_PIN);
    #endif
  #endif

  #if HAS_X2_MIN
    #if ENABLED(ENDSTOPPULLUP_XMIN)
      SET_INPUT_PULLUP(X2_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_XMIN)
      SET_INPUT_PULLDOWN(X2_MIN_PIN);
    #else
      SET_INPUT(X2_MIN_PIN);
    #endif
  #endif

  #if HAS_Y_MIN
    #if ENABLED(ENDSTOPPULLUP_YMIN)
      SET_INPUT_PULLUP(Y_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_YMIN)
      SET_INPUT_PULLDOWN(Y_MIN_PIN);
    #else
      SET_INPUT(Y_MIN_PIN);
    #endif
  #endif

  #if HAS_Y2_MIN
    #if ENABLED(ENDSTOPPULLUP_YMIN)
      SET_INPUT_PULLUP(Y2_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_YMIN)
      SET_INPUT_PULLDOWN(Y2_MIN_PIN);
    #else
      SET_INPUT(Y2_MIN_PIN);
    #endif
  #endif

  #if HAS_Z_MIN
    #if ENABLED(ENDSTOPPULLUP_ZMIN)
      SET_INPUT_PULLUP(Z_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN)
      SET_INPUT_PULLDOWN(Z_MIN_PIN);
    #else
      SET_INPUT(Z_MIN_PIN);
    #endif
  #endif

  #if HAS_Z2_MIN
    #if ENABLED(ENDSTOPPULLUP_ZMIN)
      SET_INPUT_PULLUP(Z2_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN)
      SET_INPUT_PULLDOWN(Z2_MIN_PIN);
    #else
      SET_INPUT(Z2_MIN_PIN);
    #endif
  #endif

  #if HAS_Z3_MIN
    #if ENABLED(ENDSTOPPULLUP_ZMIN)
      SET_INPUT_PULLUP(Z3_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN)
      SET_INPUT_PULLDOWN(Z3_MIN_PIN);
    #else
      SET_INPUT(Z3_MIN_PIN);
    #endif
  #endif

  #if HAS_X_MAX
    #if ENABLED(ENDSTOPPULLUP_XMAX)
      SET_INPUT_PULLUP(X_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_XMAX)
      SET_INPUT_PULLDOWN(X_MAX_PIN);
    #else
      SET_INPUT(X_MAX_PIN);
    #endif
  #endif

  #if HAS_X2_MAX
    #if ENABLED(ENDSTOPPULLUP_XMAX)
      SET_INPUT_PULLUP(X2_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_XMAX)
      SET_INPUT_PULLDOWN(X2_MAX_PIN);
    #else
      SET_INPUT(X2_MAX_PIN);
    #endif
  #endif

  #if HAS_Y_MAX
    #if ENABLED(ENDSTOPPULLUP_YMAX)
      SET_INPUT_PULLUP(Y_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_YMAX)
      SET_INPUT_PULLDOWN(Y_MAX_PIN);
    #else
      SET_INPUT(Y_MAX_PIN);
    #endif
  #endif

  #if HAS_Y2_MAX
    #if ENABLED(ENDSTOPPULLUP_YMAX)
      SET_INPUT_PULLUP(Y2_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_YMAX)
      SET_INPUT_PULLDOWN(Y2_MAX_PIN);
    #else
      SET_INPUT(Y2_MAX_PIN);
    #endif
  #endif

  #if HAS_Z_MAX
    #if ENABLED(ENDSTOPPULLUP_ZMAX)
      SET_INPUT_PULLUP(Z_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMAX)
      SET_INPUT_PULLDOWN(Z_MAX_PIN);
    #else
      SET_INPUT(Z_MAX_PIN);
    #endif
  #endif

  #if HAS_Z2_MAX
    #if ENABLED(ENDSTOPPULLUP_ZMAX)
      SET_INPUT_PULLUP(Z2_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMAX)
      SET_INPUT_PULLDOWN(Z2_MAX_PIN);
    #else
      SET_INPUT(Z2_MAX_PIN);
    #endif
  #endif

  #if HAS_Z3_MAX
    #if ENABLED(ENDSTOPPULLUP_ZMAX)
      SET_INPUT_PULLUP(Z3_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMAX)
      SET_INPUT_PULLDOWN(Z3_MAX_PIN);
    #else
      SET_INPUT(Z3_MAX_PIN);
    #endif
  #endif

  #if HAS_CALIBRATION_PIN
    #if ENABLED(CALIBRATION_PIN_PULLUP)
      SET_INPUT_PULLUP(CALIBRATION_PIN);
    #elif ENABLED(CALIBRATION_PIN_PULLDOWN)
      SET_INPUT_PULLDOWN(CALIBRATION_PIN);
    #else
      SET_INPUT(CALIBRATION_PIN);
    #endif
  #endif

  #if HAS_CUSTOM_PROBE_PIN
    #if ENABLED(ENDSTOPPULLUP_ZMIN_PROBE)
      SET_INPUT_PULLUP(Z_MIN_PROBE_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN_PROBE)
      SET_INPUT_PULLDOWN(Z_MIN_PROBE_PIN);
    #else
      SET_INPUT(Z_MIN_PROBE_PIN);
    #endif
  #endif

  #if ENABLED(ENDSTOP_INTERRUPTS_FEATURE)
    setup_endstop_interrupts();
  #endif

  // Enable endstops
  enable_globally(
    #if ENABLED(ENDSTOPS_ALWAYS_ON_DEFAULT)
      true
    #else
      false
    #endif
  );

} // Endstops::init

// Called at ~1KHz from Temperature ISR: Poll endstop state if required
void Endstops::poll() {

  #if ENABLED(PINS_DEBUGGING)
    run_monitor();  // report changes in endstop status
  #endif

  #if DISABLED(ENDSTOP_INTERRUPTS_FEATURE)
    update();
  #elif ENDSTOP_NOISE_THRESHOLD
    if (endstop_poll_count) update();
  #endif

  // Call Loadcell::HomingSafetyCheck when homing (safeguard to stop homing when loadcell samples stop comming)
  #if HAS_LOADCELL()
  bool is_homing = z_probe_enabled;
  #if HAS_TOOLCHANGER()
  is_homing = is_homing || xy_probe_enabled;
  #endif
  if (is_homing) {
    loadcell.HomingSafetyCheck();
  }
  #endif
}

void Endstops::enable_globally(const bool onoff) {
  enabled_globally = enabled = onoff;
  resync();
}

// Enable / disable endstop checking
void Endstops::enable(const bool onoff) {
  enabled = onoff;
  resync();
}

// Disable / Enable endstops based on ENDSTOPS_ONLY_FOR_HOMING and global enable
void Endstops::not_homing() {
  enabled = enabled_globally;
}

#if ENABLED(VALIDATE_HOMING_ENDSTOPS)
  // If the last move failed to trigger an endstop, call kill
  void Endstops::validate_homing_move() {
    if (trigger_state()) hit_on_purpose();
    else kill(GET_TEXT(MSG_LCD_HOMING_FAILED));
  }
#endif

// Enable / disable endstop z-probe checking
#if HAS_BED_PROBE
  void Endstops::enable_z_probe(const bool onoff) {
    z_probe_enabled = onoff;
    resync();
  }
#endif

// Enable / disable endstop xy-probe checking
#ifdef PRUSA_TOOLCHANGER
  void Endstops::enable_xy_probe(const bool onoff) {
    xy_probe_enabled = onoff;
    resync();
  }
#endif

// Get the stable endstop states when enabled
void Endstops::resync() {
  if (!abort_enabled()) return;     // If endstops/probes are disabled the loop below can hang

  #if ENABLED(ENDSTOP_INTERRUPTS_FEATURE)
    update();
  #else
    safe_delay(2);  // Wait for Temperature ISR to run at least once (runs at 1KHz)
  #endif
  #if ENDSTOP_NOISE_THRESHOLD
    while (endstop_poll_count) safe_delay(1);
  #endif
}

#if ENABLED(PINS_DEBUGGING)
  void Endstops::run_monitor() {
    if (!monitor_flag) return;
    static uint8_t monitor_count = 16;  // offset this check from the others
    monitor_count += _BV(1);            //  15 Hz
    monitor_count &= 0x7F;
    if (!monitor_count) monitor();      // report changes in endstop status
  }
#endif

void Endstops::event_handler() {
  static uint8_t prev_hit_state; // = 0
  if (hit_state && hit_state != prev_hit_state) {
    #if HAS_SPI_LCD
      char chrX = ' ', chrY = ' ', chrZ = ' ', chrP = ' ';
      #define _SET_STOP_CHAR(A,C) (chr## A = C)
    #else
      #define _SET_STOP_CHAR(A,C) ;
    #endif

    #define _ENDSTOP_HIT_ECHO(A,C) do{ \
      SERIAL_ECHOPAIR(" " STRINGIFY(A) ":", planner.triggered_position_mm(_AXIS(A))); \
      _SET_STOP_CHAR(A,C); }while(0)

    #define _ENDSTOP_HIT_TEST(A,C) \
      if (TEST(hit_state, A ##_MIN) || TEST(hit_state, A ##_MAX)) \
        _ENDSTOP_HIT_ECHO(A,C)

    #define ENDSTOP_HIT_TEST_X() _ENDSTOP_HIT_TEST(X,'X')
    #define ENDSTOP_HIT_TEST_Y() _ENDSTOP_HIT_TEST(Y,'Y')
    #define ENDSTOP_HIT_TEST_Z() _ENDSTOP_HIT_TEST(Z,'Z')

    SERIAL_ECHO_START();
    SERIAL_ECHOPGM(MSG_ENDSTOPS_HIT);
    ENDSTOP_HIT_TEST_X();
    ENDSTOP_HIT_TEST_Y();
    ENDSTOP_HIT_TEST_Z();

    #if HAS_CUSTOM_PROBE_PIN
      #define P_AXIS Z_AXIS
      if (TEST(hit_state, Z_MIN_PROBE)) _ENDSTOP_HIT_ECHO(P, 'P');
    #endif
    SERIAL_EOL();

    #if HAS_SPI_LCD
      ui.status_printf_P(0, PSTR(S_FMT " %c %c %c %c"), GET_TEXT(MSG_LCD_ENDSTOPS), chrX, chrY, chrZ, chrP);
    #endif

    #if BOTH(SD_ABORT_ON_ENDSTOP_HIT, SDSUPPORT)
      if (planner.abort_on_endstop_hit) {
        card.stopSDPrint();
        quickstop_stepper();
        thermalManager.disable_all_heaters();
        print_job_timer.stop();
      }
    #endif
  }
  prev_hit_state = hit_state;
}

static void print_es_state(const bool is_hit, PGM_P const label=nullptr) {
  if (label) serialprintPGM(label);
  SERIAL_ECHOPGM(": ");
  serialprintPGM(is_hit ? PSTR(MSG_ENDSTOP_HIT) : PSTR(MSG_ENDSTOP_OPEN));
  SERIAL_EOL();
}

void __O2 Endstops::M119() {
  #if ENABLED(BLTOUCH)
    bltouch._set_SW_mode();
  #endif
  SERIAL_ECHOLNPGM(MSG_M119_REPORT);
  #define ES_REPORT(S) print_es_state(READ(S##_PIN) != S##_ENDSTOP_INVERTING, PSTR(MSG_##S))
  #if HAS_X_MIN
    ES_REPORT(X_MIN);
  #endif
  #if HAS_X2_MIN
    ES_REPORT(X2_MIN);
  #endif
  #if HAS_X_MAX
    ES_REPORT(X_MAX);
  #endif
  #if HAS_X2_MAX
    ES_REPORT(X2_MAX);
  #endif
  #if HAS_Y_MIN
    ES_REPORT(Y_MIN);
  #endif
  #if HAS_Y2_MIN
    ES_REPORT(Y2_MIN);
  #endif
  #if HAS_Y_MAX
    ES_REPORT(Y_MAX);
  #endif
  #if HAS_Y2_MAX
    ES_REPORT(Y2_MAX);
  #endif
  #if HAS_Z_MIN
    ES_REPORT(Z_MIN);
  #endif
  #if HAS_Z2_MIN
    ES_REPORT(Z2_MIN);
  #endif
  #if HAS_Z3_MIN
    ES_REPORT(Z3_MIN);
  #endif
  #if HAS_Z_MAX
    ES_REPORT(Z_MAX);
  #endif
  #if HAS_Z2_MAX
    ES_REPORT(Z2_MAX);
  #endif
  #if HAS_Z3_MAX
    ES_REPORT(Z3_MAX);
  #endif
  #if HAS_CUSTOM_PROBE_PIN
    print_es_state(READ(Z_MIN_PROBE_PIN) != Z_MIN_PROBE_ENDSTOP_INVERTING, PSTR(MSG_Z_PROBE));
  #endif
  #if HAS_FILAMENT_SENSOR
    #if NUM_RUNOUT_SENSORS == 1
      print_es_state(READ(FIL_RUNOUT_PIN) != FIL_RUNOUT_INVERTING, PSTR(MSG_FILAMENT_RUNOUT_SENSOR));
    #else
      for (uint8_t i = 1; i <= NUM_RUNOUT_SENSORS; i++) {
        pin_t pin;
        switch (i) {
          default: continue;
          case 1: pin = FIL_RUNOUT_PIN; break;
          case 2: pin = FIL_RUNOUT2_PIN; break;
          #if NUM_RUNOUT_SENSORS > 2
            case 3: pin = FIL_RUNOUT3_PIN; break;
            #if NUM_RUNOUT_SENSORS > 3
              case 4: pin = FIL_RUNOUT4_PIN; break;
              #if NUM_RUNOUT_SENSORS > 4
                case 5: pin = FIL_RUNOUT5_PIN; break;
                #if NUM_RUNOUT_SENSORS > 5
                  case 6: pin = FIL_RUNOUT6_PIN; break;
                #endif
              #endif
            #endif
          #endif
        }
        SERIAL_ECHOPGM(MSG_FILAMENT_RUNOUT_SENSOR);
        if (i > 1) { SERIAL_CHAR(' '); SERIAL_CHAR('0' + i); }
        print_es_state(extDigitalRead(pin) != FIL_RUNOUT_INVERTING);
      }
    #endif
  #endif
  #if ENABLED(BLTOUCH)
    bltouch._reset_SW_mode();
  #endif

  #if ENABLED(JOYSTICK_DEBUG)
    joystick.report();
  #endif

} // Endstops::M119

// The following routines are called from an ISR context. It could be the temperature ISR, the
// endstop ISR or the Stepper ISR.

#define _ENDSTOP(AXIS, MINMAX) AXIS ##_## MINMAX
#define _ENDSTOP_PIN(AXIS, MINMAX) AXIS ##_## MINMAX ##_PIN
#define _ENDSTOP_INVERTING(AXIS, MINMAX) AXIS ##_## MINMAX ##_ENDSTOP_INVERTING

// Check endstops - Could be called from Temperature ISR!
void Endstops::update() {

  #if !ENDSTOP_NOISE_THRESHOLD
    if (!abort_enabled()) return;
  #endif

  #define UPDATE_ENDSTOP_BIT(AXIS, MINMAX) SET_BIT_TO(live_state, _ENDSTOP(AXIS, MINMAX), (READ(_ENDSTOP_PIN(AXIS, MINMAX)) != _ENDSTOP_INVERTING(AXIS, MINMAX)))
  #define COPY_LIVE_STATE(SRC_BIT, DST_BIT) SET_BIT_TO(live_state, DST_BIT, TEST(live_state, SRC_BIT))

  #if ENABLED(G38_PROBE_TARGET) && PIN_EXISTS(Z_MIN_PROBE) && !(CORE_IS_XY || CORE_IS_XZ)
    // If G38 command is active check Z_MIN_PROBE for ALL movement
    if (G38_move) UPDATE_ENDSTOP_BIT(Z, MIN_PROBE);
  #endif

  // With Dual X, endstops are only checked in the homing direction for the active extruder
  #if ENABLED(DUAL_X_CARRIAGE)
    #define E0_ACTIVE stepper.movement_extruder() == 0
    #define X_MIN_TEST ((X_HOME_DIR < 0 && E0_ACTIVE) || (X2_HOME_DIR < 0 && !E0_ACTIVE))
    #define X_MAX_TEST ((X_HOME_DIR > 0 && E0_ACTIVE) || (X2_HOME_DIR > 0 && !E0_ACTIVE))
  #else
    #define X_MIN_TEST true
    #define X_MAX_TEST true
  #endif

  // Use HEAD for core axes, AXIS for others
  #if CORE_IS_XY || CORE_IS_XZ
    #define X_AXIS_HEAD X_HEAD
  #else
    #define X_AXIS_HEAD X_AXIS
  #endif
  #if CORE_IS_XY || CORE_IS_YZ
    #define Y_AXIS_HEAD Y_HEAD
  #else
    #define Y_AXIS_HEAD Y_AXIS
  #endif
  #if CORE_IS_XZ || CORE_IS_YZ
    #define Z_AXIS_HEAD Z_HEAD
  #else
    #define Z_AXIS_HEAD Z_AXIS
  #endif

  /**
   * Check and update endstops
   */
  #if HAS_X_MIN && !X_SPI_SENSORLESS
    UPDATE_ENDSTOP_BIT(X, MIN);
    #if ENABLED(X_DUAL_ENDSTOPS)
      #if HAS_X2_MIN
        UPDATE_ENDSTOP_BIT(X2, MIN);
      #else
        COPY_LIVE_STATE(X_MIN, X2_MIN);
      #endif
    #endif
  #endif

  #if HAS_X_MAX && !X_SPI_SENSORLESS
    UPDATE_ENDSTOP_BIT(X, MAX);
    #if ENABLED(X_DUAL_ENDSTOPS)
      #if HAS_X2_MAX
        UPDATE_ENDSTOP_BIT(X2, MAX);
      #else
        COPY_LIVE_STATE(X_MAX, X2_MAX);
      #endif
    #endif
  #endif

  #if HAS_Y_MIN && !Y_SPI_SENSORLESS
    UPDATE_ENDSTOP_BIT(Y, MIN);
    #if ENABLED(Y_DUAL_ENDSTOPS)
      #if HAS_Y2_MIN
        UPDATE_ENDSTOP_BIT(Y2, MIN);
      #else
        COPY_LIVE_STATE(Y_MIN, Y2_MIN);
      #endif
    #endif
  #endif

  #if HAS_Y_MAX && !Y_SPI_SENSORLESS
    UPDATE_ENDSTOP_BIT(Y, MAX);
    #if ENABLED(Y_DUAL_ENDSTOPS)
      #if HAS_Y2_MAX
        UPDATE_ENDSTOP_BIT(Y2, MAX);
      #else
        COPY_LIVE_STATE(Y_MAX, Y2_MAX);
      #endif
    #endif
  #endif

  #if HAS_Z_MIN && !Z_SPI_SENSORLESS
    UPDATE_ENDSTOP_BIT(Z, MIN);
    #if Z_MULTI_ENDSTOPS
      #if HAS_Z2_MIN
        UPDATE_ENDSTOP_BIT(Z2, MIN);
      #else
        COPY_LIVE_STATE(Z_MIN, Z2_MIN);
      #endif
      #if ENABLED(Z_TRIPLE_ENDSTOPS)
        #if HAS_Z3_MIN
          UPDATE_ENDSTOP_BIT(Z3, MIN);
        #else
          COPY_LIVE_STATE(Z_MIN, Z3_MIN);
        #endif
      #endif
    #endif
  #endif

  // When closing the gap check the enabled probe
  #if HAS_CUSTOM_PROBE_PIN
    UPDATE_ENDSTOP_BIT(Z, MIN_PROBE);
  #endif

  #if HAS_Z_MAX && !Z_SPI_SENSORLESS
    // Check both Z dual endstops
    #if Z_MULTI_ENDSTOPS
      UPDATE_ENDSTOP_BIT(Z, MAX);
      #if HAS_Z2_MAX
        UPDATE_ENDSTOP_BIT(Z2, MAX);
      #else
        COPY_LIVE_STATE(Z_MAX, Z2_MAX);
      #endif
      #if ENABLED(Z_TRIPLE_ENDSTOPS)
        #if HAS_Z3_MAX
          UPDATE_ENDSTOP_BIT(Z3, MAX);
        #else
          COPY_LIVE_STATE(Z_MAX, Z3_MAX);
        #endif
      #endif
    #elif !HAS_CUSTOM_PROBE_PIN || Z_MAX_PIN != Z_MIN_PROBE_PIN
      // If this pin isn't the bed probe it's the Z endstop
      UPDATE_ENDSTOP_BIT(Z, MAX);
    #endif
  #endif

  #if ENDSTOP_NOISE_THRESHOLD

    /**
     * Filtering out noise on endstops requires a delayed decision. Let's assume, due to noise,
     * that 50% of endstop signal samples are good and 50% are bad (assuming normal distribution
     * of random noise). Then the first sample has a 50% chance to be good or bad. The 2nd sample
     * also has a 50% chance to be good or bad. The chances of 2 samples both being bad becomes
     * 50% of 50%, or 25%. That was the previous implementation of Marlin endstop handling. It
     * reduces chances of bad readings in half, at the cost of 1 extra sample period, but chances
     * still exist. The only way to reduce them further is to increase the number of samples.
     * To reduce the chance to 1% (1/128th) requires 7 samples (adding 7ms of delay).
     */
    static esbits_t old_live_state;
    if (old_live_state != live_state) {
      endstop_poll_count = ENDSTOP_NOISE_THRESHOLD;
      old_live_state = live_state;
    }
    else if (endstop_poll_count && !--endstop_poll_count)
      validated_live_state = live_state;

    if (!abort_enabled()) return;

  #endif

  // Test the current status of an endstop
  #define TEST_ENDSTOP(ENDSTOP) (TEST(state(), ENDSTOP))

  // Record endstop was hit
  #define _ENDSTOP_HIT(AXIS, MINMAX) SBI(hit_state, _ENDSTOP(AXIS, MINMAX))

  // Call the endstop triggered routine for single endstops
  #define PROCESS_ENDSTOP(AXIS,MINMAX) do { \
    if (TEST_ENDSTOP(_ENDSTOP(AXIS, MINMAX))) { \
      _ENDSTOP_HIT(AXIS, MINMAX); \
      planner.endstop_triggered(_AXIS(AXIS)); \
    } \
  }while(0)

  // Call the endstop triggered routine for dual endstops
  #define PROCESS_DUAL_ENDSTOP(AXIS1, AXIS2, MINMAX) do { \
    const byte dual_hit = TEST_ENDSTOP(_ENDSTOP(AXIS1, MINMAX)) | (TEST_ENDSTOP(_ENDSTOP(AXIS2, MINMAX)) << 1); \
    if (dual_hit) { \
      _ENDSTOP_HIT(AXIS1, MINMAX); \
      /* if not performing home or if both endstops were trigged during homing... */ \
      if (!stepper.separate_multi_axis || dual_hit == 0b11) \
        planner.endstop_triggered(_AXIS(AXIS1)); \
    } \
  }while(0)

  #define PROCESS_TRIPLE_ENDSTOP(AXIS1, AXIS2, AXIS3, MINMAX) do { \
    const byte triple_hit = TEST_ENDSTOP(_ENDSTOP(AXIS1, MINMAX)) | (TEST_ENDSTOP(_ENDSTOP(AXIS2, MINMAX)) << 1) | (TEST_ENDSTOP(_ENDSTOP(AXIS3, MINMAX)) << 2); \
    if (triple_hit) { \
      _ENDSTOP_HIT(AXIS1, MINMAX); \
      /* if not performing home or if both endstops were trigged during homing... */ \
      if (!stepper.separate_multi_axis || triple_hit == 0b111) \
        planner.endstop_triggered(_AXIS(AXIS1)); \
    } \
  }while(0)

  #if ENABLED(G38_PROBE_TARGET) && PIN_EXISTS(Z_MIN_PROBE) && !(CORE_IS_XY || CORE_IS_XZ)
    #if ENABLED(G38_PROBE_AWAY)
      #define _G38_OPEN_STATE (G38_move >= 4)
    #else
      #define _G38_OPEN_STATE LOW
    #endif
    // If G38 command is active check Z_MIN_PROBE for ALL movement
    if (G38_move && TEST_ENDSTOP(_ENDSTOP(Z, MIN_PROBE)) != _G38_OPEN_STATE) {
           if (stepper.axis_is_moving(X_AXIS)) { _ENDSTOP_HIT(X, MIN); planner.endstop_triggered(X_AXIS); }
      else if (stepper.axis_is_moving(Y_AXIS)) { _ENDSTOP_HIT(Y, MIN); planner.endstop_triggered(Y_AXIS); }
      else if (stepper.axis_is_moving(Z_AXIS)) { _ENDSTOP_HIT(Z, MIN); planner.endstop_triggered(Z_AXIS); }
      G38_did_trigger = true;
    }
  #endif

  // Now, we must signal, after validation, if an endstop limit is pressed or not
  if (stepper.axis_is_moving(X_AXIS)) {
    if (stepper.motor_direction(X_AXIS_HEAD)) { // -direction
      #if HAS_X_MIN || (X_SPI_SENSORLESS && X_HOME_DIR < 0)
        #if ENABLED(X_DUAL_ENDSTOPS)
          PROCESS_DUAL_ENDSTOP(X, X2, MIN);
        #else
          if (X_MIN_TEST) PROCESS_ENDSTOP(X, MIN);
        #endif
      #endif
    }
    else { // +direction
      #if HAS_X_MAX || (X_SPI_SENSORLESS && X_HOME_DIR > 0)
        #if ENABLED(X_DUAL_ENDSTOPS)
          PROCESS_DUAL_ENDSTOP(X, X2, MAX);
        #else
          if (X_MAX_TEST) PROCESS_ENDSTOP(X, MAX);
        #endif
      #endif
    }
  }

// Handle XY probing
#if BOARD_IS_XLBUDDY()
  // TODO: This does not clean the endstop bits on xy_probe disable. It cannot as it might clear the real endstops.
  // The hit_on_purpose is supposed to be called cleaning the bits.
  if(stepper.axis_is_moving(X_AXIS)) {
    if(xy_probe_enabled) {
      SET_BIT_TO(live_state, _ENDSTOP(X, MIN), READ(MARLIN_PIN(XY_PROBE)) != XY_PROBE_ENDSTOP_INVERTING && stepper.motor_direction(X_AXIS_HEAD));
      PROCESS_ENDSTOP(X, MIN);
      SET_BIT_TO(live_state, _ENDSTOP(X, MAX), READ(MARLIN_PIN(XY_PROBE)) != XY_PROBE_ENDSTOP_INVERTING && !stepper.motor_direction(X_AXIS_HEAD));
      PROCESS_ENDSTOP(X, MAX);
    }
  }
  if(stepper.axis_is_moving(Y_AXIS)) {
    if(xy_probe_enabled) {
      SET_BIT_TO(live_state, _ENDSTOP(Y, MIN), READ(MARLIN_PIN(XY_PROBE)) != XY_PROBE_ENDSTOP_INVERTING && stepper.motor_direction(Y_AXIS_HEAD));
      PROCESS_ENDSTOP(Y, MIN);
      SET_BIT_TO(live_state, _ENDSTOP(Y, MAX), READ(MARLIN_PIN(XY_PROBE)) != XY_PROBE_ENDSTOP_INVERTING && !stepper.motor_direction(Y_AXIS_HEAD));
      PROCESS_ENDSTOP(Y, MAX);
    }
  }
#endif


  if (stepper.axis_is_moving(Y_AXIS)) {
    if (stepper.motor_direction(Y_AXIS_HEAD)) { // -direction
      #if HAS_Y_MIN || (Y_SPI_SENSORLESS && Y_HOME_DIR < 0)
        #if ENABLED(Y_DUAL_ENDSTOPS)
          PROCESS_DUAL_ENDSTOP(Y, Y2, MIN);
        #else
          PROCESS_ENDSTOP(Y, MIN);
        #endif
      #endif
    }
    else { // +direction
      #if HAS_Y_MAX || (Y_SPI_SENSORLESS && Y_HOME_DIR > 0)
        #if ENABLED(Y_DUAL_ENDSTOPS)
          PROCESS_DUAL_ENDSTOP(Y, Y2, MAX);
        #else
          PROCESS_ENDSTOP(Y, MAX);
        #endif
      #endif
    }
  }

  if (stepper.axis_is_moving(Z_AXIS)) {
    if (stepper.motor_direction(Z_AXIS_HEAD)) { // Z -direction. Gantry down, bed up.
      #if HAS_Z_MIN || (Z_SPI_SENSORLESS && Z_HOME_DIR < 0)
        #if ENABLED(Z_TRIPLE_ENDSTOPS)
          PROCESS_TRIPLE_ENDSTOP(Z, Z2, Z3, MIN);
        #elif ENABLED(Z_DUAL_ENDSTOPS)
          PROCESS_DUAL_ENDSTOP(Z, Z2, MIN);
        #else
          #if HAS_LOADCELL()
            PROCESS_ENDSTOP(Z, MIN); // Loadcell is disabled elsewhere
          #elif ENABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
            if (z_probe_enabled) PROCESS_ENDSTOP(Z, MIN);
          #elif HAS_CUSTOM_PROBE_PIN
            if (!z_probe_enabled) PROCESS_ENDSTOP(Z, MIN);
          #else
            PROCESS_ENDSTOP(Z, MIN);
          #endif
        #endif
      #endif

      // When closing the gap check the enabled probe
      #if HAS_CUSTOM_PROBE_PIN
        if (z_probe_enabled) PROCESS_ENDSTOP(Z, MIN_PROBE);
      #endif
    }
    else { // Z +direction. Gantry up, bed down.
      #if HAS_Z_MAX || (Z_SPI_SENSORLESS && Z_HOME_DIR > 0)
        #if ENABLED(Z_TRIPLE_ENDSTOPS)
          PROCESS_TRIPLE_ENDSTOP(Z, Z2, Z3, MAX);
        #elif ENABLED(Z_DUAL_ENDSTOPS)
          PROCESS_DUAL_ENDSTOP(Z, Z2, MAX);
        #elif !HAS_CUSTOM_PROBE_PIN || Z_MAX_PIN != Z_MIN_PROBE_PIN
          // If this pin is not hijacked for the bed probe
          // then it belongs to the Z endstop
          PROCESS_ENDSTOP(Z, MAX);
        #endif
      #endif
    }
  }
} // Endstops::update()

#if ENABLED(SPI_ENDSTOPS)

  #define X_STOP (X_HOME_DIR < 0 ? X_MIN : X_MAX)
  #define Y_STOP (Y_HOME_DIR < 0 ? Y_MIN : Y_MAX)
  #define Z_STOP (Z_HOME_DIR < 0 ? Z_MIN : Z_MAX)

  bool Endstops::tmc_spi_homing_check() {
    bool hit = false;
    #if X_SPI_SENSORLESS
      if (tmc_spi_homing.x && stepperX.test_stall_status()) {
        SBI(live_state, X_STOP);
        hit = true;
      }
    #endif
    #if Y_SPI_SENSORLESS
      if (tmc_spi_homing.y && stepperY.test_stall_status()) {
        SBI(live_state, Y_STOP);
        hit = true;
      }
    #endif
    #if Z_SPI_SENSORLESS
      if (tmc_spi_homing.z && stepperZ.test_stall_status()) {
        SBI(live_state, Z_STOP);
        hit = true;
      }
    #endif
    return hit;
  }

  void Endstops::clear_endstop_state() {
    #if X_SPI_SENSORLESS
      CBI(live_state, X_STOP);
    #endif
    #if Y_SPI_SENSORLESS
      CBI(live_state, Y_STOP);
    #endif
    #if Z_SPI_SENSORLESS
      CBI(live_state, Z_STOP);
    #endif
  }

#endif // SPI_ENDSTOPS

#if ENABLED(PINS_DEBUGGING)

  bool Endstops::monitor_flag = false;

  /**
   * Monitor Endstops and Z Probe for changes
   *
   * If a change is detected then the LED is toggled and
   * a message is sent out the serial port.
   *
   * Yes, we could miss a rapid back & forth change but
   * that won't matter because this is all manual.
   */
  void Endstops::monitor() {

    static uint16_t old_live_state_local = 0;
    static uint8_t local_LED_status = 0;
    uint16_t live_state_local = 0;

    #define ES_GET_STATE(S) if (READ(S##_PIN)) SBI(live_state_local, S)

    #if HAS_X_MIN
      ES_GET_STATE(X_MIN);
    #endif
    #if HAS_X_MAX
      ES_GET_STATE(X_MAX);
    #endif
    #if HAS_Y_MIN
      ES_GET_STATE(Y_MIN);
    #endif
    #if HAS_Y_MAX
      ES_GET_STATE(Y_MAX);
    #endif
    #if HAS_Z_MIN
      ES_GET_STATE(Z_MIN);
    #endif
    #if HAS_Z_MAX
      ES_GET_STATE(Z_MAX);
    #endif
    #if HAS_Z_MIN_PROBE_PIN
      ES_GET_STATE(Z_MIN_PROBE);
    #endif
    #if HAS_X2_MIN
      ES_GET_STATE(X2_MIN);
    #endif
    #if HAS_X2_MAX
      ES_GET_STATE(X2_MAX);
    #endif
    #if HAS_Y2_MIN
      ES_GET_STATE(Y2_MIN);
    #endif
    #if HAS_Y2_MAX
      ES_GET_STATE(Y2_MAX);
    #endif
    #if HAS_Z2_MIN
      ES_GET_STATE(Z2_MIN);
    #endif
    #if HAS_Z2_MAX
      ES_GET_STATE(Z2_MAX);
    #endif
    #if HAS_Z3_MIN
      ES_GET_STATE(Z3_MIN);
    #endif
    #if HAS_Z3_MAX
      ES_GET_STATE(Z3_MAX);
    #endif

    uint16_t endstop_change = live_state_local ^ old_live_state_local;
    #define ES_REPORT_CHANGE(S) if (TEST(endstop_change, S)) SERIAL_ECHOPAIR("  " STRINGIFY(S) ":", TEST(live_state_local, S))

    if (endstop_change) {
      #if HAS_X_MIN
        ES_REPORT_CHANGE(X_MIN);
      #endif
      #if HAS_X_MAX
        ES_REPORT_CHANGE(X_MAX);
      #endif
      #if HAS_Y_MIN
        ES_REPORT_CHANGE(Y_MIN);
      #endif
      #if HAS_Y_MAX
        ES_REPORT_CHANGE(Y_MAX);
      #endif
      #if HAS_Z_MIN
        ES_REPORT_CHANGE(Z_MIN);
      #endif
      #if HAS_Z_MAX
        ES_REPORT_CHANGE(Z_MAX);
      #endif
      #if HAS_Z_MIN_PROBE_PIN
        ES_REPORT_CHANGE(Z_MIN_PROBE);
      #endif
      #if HAS_X2_MIN
        ES_REPORT_CHANGE(X2_MIN);
      #endif
      #if HAS_X2_MAX
        ES_REPORT_CHANGE(X2_MAX);
      #endif
      #if HAS_Y2_MIN
        ES_REPORT_CHANGE(Y2_MIN);
      #endif
      #if HAS_Y2_MAX
        ES_REPORT_CHANGE(Y2_MAX);
      #endif
      #if HAS_Z2_MIN
        ES_REPORT_CHANGE(Z2_MIN);
      #endif
      #if HAS_Z2_MAX
        ES_REPORT_CHANGE(Z2_MAX);
      #endif
      #if HAS_Z3_MIN
        ES_REPORT_CHANGE(Z3_MIN);
      #endif
      #if HAS_Z3_MAX
        ES_REPORT_CHANGE(Z3_MAX);
      #endif
      SERIAL_ECHOLNPGM("\n");
      analogWrite(pin_t(LED_PIN), local_LED_status);
      local_LED_status ^= 255;
      old_live_state_local = live_state_local;
    }
  }

#endif // PINS_DEBUGGING
