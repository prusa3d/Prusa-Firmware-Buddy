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
#include "temperature.h"
#include "../lcd/ultralcd.h"
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include <option/has_indx.h>

#if ENABLED(ENDSTOP_INTERRUPTS_FEATURE)
  #include HAL_PATH(../HAL, endstop_interrupts.h)
#endif

#if HAS_LOADCELL()
  #include "loadcell.hpp"
#endif

Endstops endstops;

// private:

bool Endstops::enabled, Endstops::enabled_globally; // Initialized by settings.load()
volatile uint8_t Endstops::hit_state;

Endstops::esbits_t Endstops::live_state = 0;

#if HAS_BED_PROBE
  std::atomic<bool> Endstops::z_probe_enabled { false };
#endif

#if HAS_TOOLCHANGER()
  std::atomic<bool> Endstops::xy_probe_enabled { false };
#endif

// Initialized by settings.load()
#if ENABLED(Z_TRIPLE_ENDSTOPS)
  // #error dead code found by automatic analyses (see BFW-5461)
  float Endstops::z2_endstop_adj;
  float Endstops::z3_endstop_adj;
#endif

#if ENABLED(IMPROVE_HOMING_RELIABILITY) && HOMING_SG_GUARD_DURATION > 0
  // #error dead code found by automatic analyses (see BFW-5461)
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
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(X_MIN_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(X_MIN_PIN);
    #endif
  #endif

  #if HAS_Y_MIN
    #if ENABLED(ENDSTOPPULLUP_YMIN)
      SET_INPUT_PULLUP(Y_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_YMIN)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Y_MIN_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Y_MIN_PIN);
    #endif
  #endif

  #if HAS_Z_MIN
    #if ENABLED(ENDSTOPPULLUP_ZMIN)
      SET_INPUT_PULLUP(Z_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Z_MIN_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Z_MIN_PIN);
    #endif
  #endif

  #if HAS_Z2_MIN
    // #error dead code found by automatic analyses (see BFW-5461)
    #if ENABLED(ENDSTOPPULLUP_ZMIN)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLUP(Z2_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Z2_MIN_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Z2_MIN_PIN);
    #endif
  #endif

  #if HAS_Z3_MIN
    // #error dead code found by automatic analyses (see BFW-5461)
    #if ENABLED(ENDSTOPPULLUP_ZMIN)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLUP(Z3_MIN_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Z3_MIN_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Z3_MIN_PIN);
    #endif
  #endif

  #if HAS_X_MAX
    #if ENABLED(ENDSTOPPULLUP_XMAX)
      SET_INPUT_PULLUP(X_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_XMAX)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(X_MAX_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(X_MAX_PIN);
    #endif
  #endif

  #if HAS_Y_MAX
    #if ENABLED(ENDSTOPPULLUP_YMAX)
      SET_INPUT_PULLUP(Y_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_YMAX)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Y_MAX_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Y_MAX_PIN);
    #endif
  #endif

  #if HAS_Z_MAX
    #if ENABLED(ENDSTOPPULLUP_ZMAX)
      SET_INPUT_PULLUP(Z_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMAX)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Z_MAX_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Z_MAX_PIN);
    #endif
  #endif

  #if HAS_Z2_MAX
    // #error dead code found by automatic analyses (see BFW-5461)
    #if ENABLED(ENDSTOPPULLUP_ZMAX)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLUP(Z2_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMAX)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Z2_MAX_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Z2_MAX_PIN);
    #endif
  #endif

  #if HAS_Z3_MAX
    // #error dead code found by automatic analyses (see BFW-5461)
    #if ENABLED(ENDSTOPPULLUP_ZMAX)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLUP(Z3_MAX_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMAX)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Z3_MAX_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Z3_MAX_PIN);
    #endif
  #endif

  #if HAS_CUSTOM_PROBE_PIN
    // #error dead code found by automatic analyses (see BFW-5461)
    #if ENABLED(ENDSTOPPULLUP_ZMIN_PROBE)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLUP(Z_MIN_PROBE_PIN);
    #elif ENABLED(ENDSTOPPULLDOWN_ZMIN_PROBE)
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT_PULLDOWN(Z_MIN_PROBE_PIN);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      SET_INPUT(Z_MIN_PROBE_PIN);
    #endif
  #endif

  #if ENABLED(ENDSTOP_INTERRUPTS_FEATURE)
    setup_endstop_interrupts();
  #endif

  // Enable endstops
  enable_globally(
    #if ENABLED(ENDSTOPS_ALWAYS_ON_DEFAULT)
      // #error dead code found by automatic analyses (see BFW-5461)
      true
    #else
      false
    #endif
  );

} // Endstops::init

// Called at ~1KHz from Temperature ISR: Poll endstop state if required
void Endstops::poll() {
  #if DISABLED(ENDSTOP_INTERRUPTS_FEATURE)
    // #error dead code found by automatic analyses (see BFW-5461)
    update();
  #endif

  // Stop the probe if loadcell samples stop arriving (self-gates to probe sessions).
  #if HAS_LOADCELL()
    loadcell.HomingSafetyCheck();
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
  // #error dead code found by automatic analyses (see BFW-5461)
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
#if HAS_TOOLCHANGER()
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
    // #error dead code found by automatic analyses (see BFW-5461)
    safe_delay(2);  // Wait for Temperature ISR to run at least once (runs at 1KHz)
  #endif
}

void Endstops::event_handler() {
  static uint8_t prev_hit_state; // = 0
  if (hit_state && hit_state != prev_hit_state) {
    #define _SET_STOP_CHAR(A,C) ;

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
      // #error dead code found by automatic analyses (see BFW-5461)
      #define P_AXIS Z_AXIS
      if (TEST(hit_state, Z_MIN_PROBE)) _ENDSTOP_HIT_ECHO(P, 'P');
    #endif
    SERIAL_EOL();
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
  SERIAL_ECHOLNPGM(MSG_M119_REPORT);
  #define ES_REPORT(S) print_es_state(READ(S##_PIN) != S##_ENDSTOP_INVERTING, PSTR(MSG_##S))
  #if HAS_X_MIN
    ES_REPORT(X_MIN);
  #endif
  #if HAS_X_MAX
    ES_REPORT(X_MAX);
  #endif
  #if HAS_Y_MIN
    ES_REPORT(Y_MIN);
  #endif
  #if HAS_Y_MAX
    ES_REPORT(Y_MAX);
  #endif
  #if HAS_Z_MIN
    ES_REPORT(Z_MIN);
  #endif
  #if HAS_Z2_MIN
    // #error dead code found by automatic analyses (see BFW-5461)
    ES_REPORT(Z2_MIN);
  #endif
  #if HAS_Z3_MIN
    // #error dead code found by automatic analyses (see BFW-5461)
    ES_REPORT(Z3_MIN);
  #endif
  #if HAS_Z_MAX
    ES_REPORT(Z_MAX);
  #endif
  #if HAS_Z2_MAX
    // #error dead code found by automatic analyses (see BFW-5461)
    ES_REPORT(Z2_MAX);
  #endif
  #if HAS_Z3_MAX
    // #error dead code found by automatic analyses (see BFW-5461)
    ES_REPORT(Z3_MAX);
  #endif
  #if HAS_CUSTOM_PROBE_PIN
    // #error dead code found by automatic analyses (see BFW-5461)
    print_es_state(READ(Z_MIN_PROBE_PIN) != Z_MIN_PROBE_ENDSTOP_INVERTING, PSTR(MSG_Z_PROBE));
  #endif
} // Endstops::M119

// The following routines are called from an ISR context. It could be the temperature ISR, the
// endstop ISR or the Stepper ISR.

#define _ENDSTOP(AXIS, MINMAX) AXIS ##_## MINMAX
#define _ENDSTOP_PIN(AXIS, MINMAX) AXIS ##_## MINMAX ##_PIN
#define _ENDSTOP_INVERTING(AXIS, MINMAX) AXIS ##_## MINMAX ##_ENDSTOP_INVERTING

// Check endstops - Could be called from Temperature ISR!
void Endstops::update() {

    if (!abort_enabled()) return;

  #define UPDATE_ENDSTOP_BIT(AXIS, MINMAX) SET_BIT_TO(live_state, _ENDSTOP(AXIS, MINMAX), (READ(_ENDSTOP_PIN(AXIS, MINMAX)) != _ENDSTOP_INVERTING(AXIS, MINMAX)))
  #define COPY_LIVE_STATE(SRC_BIT, DST_BIT) SET_BIT_TO(live_state, DST_BIT, TEST(live_state, SRC_BIT))

  #if ENABLED(G38_PROBE_TARGET) && PIN_EXISTS(Z_MIN_PROBE) && !(CORE_IS_XY || CORE_IS_XZ)
    // #error dead code found by automatic analyses (see BFW-5461)
    // If G38 command is active check Z_MIN_PROBE for ALL movement
    if (G38_move) UPDATE_ENDSTOP_BIT(Z, MIN_PROBE);
  #endif

  #define X_MIN_TEST true
  #define X_MAX_TEST true

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
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_AXIS_HEAD Z_HEAD
  #else
    #define Z_AXIS_HEAD Z_AXIS
  #endif

  /**
   * Check and update endstops
   */
  #if HAS_X_MIN
    UPDATE_ENDSTOP_BIT(X, MIN);
  #endif

  #if HAS_X_MAX
    UPDATE_ENDSTOP_BIT(X, MAX);
  #endif

  #if HAS_Y_MIN
    UPDATE_ENDSTOP_BIT(Y, MIN);
  #endif

  #if HAS_Y_MAX
    UPDATE_ENDSTOP_BIT(Y, MAX);
  #endif

  #if HAS_Z_MIN
    UPDATE_ENDSTOP_BIT(Z, MIN);
    #if ENABLED(Z_TRIPLE_ENDSTOPS)
      // #error dead code found by automatic analyses (see BFW-5461)
      #if HAS_Z2_MIN
        // #error dead code found by automatic analyses (see BFW-5461)
        UPDATE_ENDSTOP_BIT(Z2, MIN);
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        COPY_LIVE_STATE(Z_MIN, Z2_MIN);
      #endif
      #if HAS_Z3_MIN
        // #error dead code found by automatic analyses (see BFW-5461)
        UPDATE_ENDSTOP_BIT(Z3, MIN);
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        COPY_LIVE_STATE(Z_MIN, Z3_MIN);
      #endif
    #endif
  #endif

  // When closing the gap check the enabled probe
  #if HAS_CUSTOM_PROBE_PIN
    // #error dead code found by automatic analyses (see BFW-5461)
    UPDATE_ENDSTOP_BIT(Z, MIN_PROBE);
  #endif

  #if HAS_Z_MAX
    // Check both Z dual endstops
    #if ENABLED(Z_TRIPLE_ENDSTOPS)
      // #error dead code found by automatic analyses (see BFW-5461)
      UPDATE_ENDSTOP_BIT(Z, MAX);
      #if HAS_Z2_MAX
        // #error dead code found by automatic analyses (see BFW-5461)
        UPDATE_ENDSTOP_BIT(Z2, MAX);
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        COPY_LIVE_STATE(Z_MAX, Z2_MAX);
      #endif
      #if HAS_Z3_MAX
        // #error dead code found by automatic analyses (see BFW-5461)
        UPDATE_ENDSTOP_BIT(Z3, MAX);
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        COPY_LIVE_STATE(Z_MAX, Z3_MAX);
      #endif
    #elif !HAS_CUSTOM_PROBE_PIN || Z_MAX_PIN != Z_MIN_PROBE_PIN
      // If this pin isn't the bed probe it's the Z endstop
      UPDATE_ENDSTOP_BIT(Z, MAX);
    #endif
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
    // #error dead code found by automatic analyses (see BFW-5461)
    #if ENABLED(G38_PROBE_AWAY)
      // #error dead code found by automatic analyses (see BFW-5461)
      #define _G38_OPEN_STATE (G38_move >= 4)
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
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
      #if HAS_X_MIN
        if (X_MIN_TEST) PROCESS_ENDSTOP(X, MIN);
      #endif
    }
    else { // +direction
      #if HAS_X_MAX
        if (X_MAX_TEST) PROCESS_ENDSTOP(X, MAX);
      #endif
    }
  }

// Handle XY probing
#if BOARD_IS_XLBUDDY() || HAS_INDX()
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
      #if HAS_Y_MIN
        PROCESS_ENDSTOP(Y, MIN);
      #endif
    }
    else { // +direction
      #if HAS_Y_MAX
        PROCESS_ENDSTOP(Y, MAX);
      #endif
    }
  }

  if (stepper.axis_is_moving(Z_AXIS)) {
    if (stepper.motor_direction(Z_AXIS_HEAD)) { // Z -direction. Gantry down, bed up.
      #if HAS_Z_MIN
        #if ENABLED(Z_TRIPLE_ENDSTOPS)
          // #error dead code found by automatic analyses (see BFW-5461)
          PROCESS_TRIPLE_ENDSTOP(Z, Z2, Z3, MIN);
        #else
          #if HAS_LOADCELL()
            PROCESS_ENDSTOP(Z, MIN); // Loadcell is disabled elsewhere
          #elif ENABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
            if (z_probe_enabled) PROCESS_ENDSTOP(Z, MIN);
          #elif HAS_CUSTOM_PROBE_PIN
            // #error dead code found by automatic analyses (see BFW-5461)
            if (!z_probe_enabled) PROCESS_ENDSTOP(Z, MIN);
          #else
            // #error dead code found by automatic analyses (see BFW-5461)
            PROCESS_ENDSTOP(Z, MIN);
          #endif
        #endif
      #endif

      // When closing the gap check the enabled probe
      #if HAS_CUSTOM_PROBE_PIN
        // #error dead code found by automatic analyses (see BFW-5461)
        if (z_probe_enabled) PROCESS_ENDSTOP(Z, MIN_PROBE);
      #endif
    }
    else { // Z +direction. Gantry up, bed down.
      #if HAS_Z_MAX
        #if ENABLED(Z_TRIPLE_ENDSTOPS)
          // #error dead code found by automatic analyses (see BFW-5461)
          PROCESS_TRIPLE_ENDSTOP(Z, Z2, Z3, MAX);
        #elif !HAS_CUSTOM_PROBE_PIN || Z_MAX_PIN != Z_MIN_PROBE_PIN
          // If this pin is not hijacked for the bed probe
          // then it belongs to the Z endstop
          PROCESS_ENDSTOP(Z, MAX);
        #endif
      #endif
    }
  }
} // Endstops::update()
