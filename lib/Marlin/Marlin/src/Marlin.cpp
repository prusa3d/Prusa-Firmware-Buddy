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
 * About Marlin
 *
 * This firmware is a mashup between Sprinter and grbl.
 *  - https://github.com/kliment/Sprinter
 *  - https://github.com/grbl/grbl
 */

#include "Marlin.h"

#include <option/has_phase_stepping.h>
#if HAS_PHASE_STEPPING()
  #include "feature/phase_stepping/phase_stepping.hpp"
#endif

#include <option/has_burst_stepping.h>
#if HAS_BURST_STEPPING()
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "feature/phase_stepping/burst_stepper.hpp"
#endif

#include "core/utility.h"
#include "lcd/ultralcd.h"
#include <lcd/extensible_ui/ui_api.h>

#include <option/has_planner.h>
#if HAS_PLANNER()
  #include "module/motion.h"
  #include "module/planner.h"
  #include "module/stepper.h"
  #include "module/endstops.h"
  #include "feature/input_shaper/input_shaper_config.hpp"
  #include "feature/pressure_advance/pressure_advance_config.hpp"
#endif

#include "module/probe.h"
#include "module/temperature.h"
#include "module/configuration_store.h"
#include "module/printcounter.h"

#include <option/board_is_master_board.h>
#if BOARD_IS_MASTER_BOARD()
#include "pause_stubbed.hpp"
#endif

#include "HAL/shared/Delay.h"

#include "module/stepper/indirection.h"
#include "feature/motordriver_util.h"

#include <feature/cork/tracker.hpp>

#include <math.h>

#include "gcode/gcode.h"
#include "gcode/parser.h"
#include "gcode/queue.h"

#if ENABLED(HOST_ACTION_COMMANDS)
  #include "feature/host_actions.h"
#endif

#if ENABLED(NOZZLE_LOAD_CELL)
  #include "loadcell.hpp"
  #include "feature/prusa/e-stall_detector.h"
#endif

#if ENABLED(G38_PROBE_TARGET)
  // #error dead code found by automatic analyses (see BFW-5461)
  uint8_t G38_move; // = 0
  bool G38_did_trigger; // = false
#endif

#if HAS_LEVELING
  #include "feature/bedlevel/bedlevel.h"
#endif

#include <option/has_pause.h>
#if HAS_PAUSE()
  #include "feature/pause.h"
#endif

#if HAS_CASE_LIGHT
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "feature/caselight.h"
#endif

#include <option/has_mmu2.h>
#if HAS_MMU2()
  #include "feature/prusa/MMU2/mmu2_mk4.h"
#endif

// For M109 and M190, this flag may be cleared (by M108) to exit the wait loop
bool wait_for_heatup = true;

// For M0/M1, this flag may be cleared (by M108) to exit the wait-for-user loop
#if HAS_RESUME_CONTINUE
  bool wait_for_user; // = false;
#endif

#if HAS_AUTO_REPORTING || ENABLED(HOST_KEEPALIVE_FEATURE)
  bool suspend_auto_report; // = false
#endif

uint16_t job_id = 0;

/**
 * ***************************************************************************
 * ******************************** FUNCTIONS ********************************
 * ***************************************************************************
 */

void setup_killpin() {
  #if HAS_KILL
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_INPUT_PULLUP(KILL_PIN);
  #endif
}

void setup_powerhold() {
  #if HAS_SUICIDE
    // #error dead code found by automatic analyses (see BFW-5461)
    OUT_WRITE(SUICIDE_PIN, HIGH);
  #endif
  #if HAS_POWER_SWITCH
    #if ENABLED(PS_DEFAULT_OFF)
      // #error dead code found by automatic analyses (see BFW-5461)
      powersupply_on = true;  PSU_OFF();
    #else
      powersupply_on = false; PSU_ON();
    #endif
  #endif
}

/**
 * Stepper Reset (RigidBoard, et.al.)
 */
#if HAS_STEPPER_RESET
  // #error dead code found by automatic analyses (see BFW-5461)
  void disableStepperDrivers() { OUT_WRITE(STEPPER_RESET_PIN, LOW); } // Drive down to keep motor driver chips in reset
  void enableStepperDrivers()  { SET_INPUT(STEPPER_RESET_PIN); }      // Set to input, allowing pullups to pull the pin high
#endif

#if HAS_PLANNER()
  void quickstop_stepper() {
    planner.quick_stop();
    planner.synchronize();
    set_current_from_steppers_for_axis(ALL_AXES_ENUM);
    sync_plan_position();
  }
#endif /* HAS_PLANNER() */

void enable_e_steppers() {
  enable_E(0); enable_E(1); enable_E(2); enable_E(3); enable_E(4); enable_E(5);
}

void enable_all_steppers() {
  #if ENABLED(AUTO_POWER_CONTROL)
    powerManager.power_on();
  #endif
  enable_XY();
  enable_Z();
  enable_e_steppers();
}

void disable_e_steppers() {
  disable_E(0); disable_E(1); disable_E(2); disable_E(3); disable_E(4); disable_E(5);
}

void disable_e_stepper(const uint8_t e) {
  disable_E(e);
}

void disable_xy_steppers() {
  disable_XY();
}

void disable_all_steppers() {
  disable_XY();
  disable_Z();
  disable_e_steppers();
}

/**
 * Printing is active when the print job timer is running
 */
bool printingIsActive() {
  return print_job_timer.isRunning();
}

/**
 * Printing is paused according to SD or host indicators
 */
bool printingIsPaused() {
  return print_job_timer.isPaused();
}

/**
 * Whether any heater (bed or hotend) has target temperature != 0
 */
bool anyHeatherIsActive() {
  bool active = false;
  #if HAS_HEATED_BED
    active |= thermalManager.degTargetBed() != 0;
  #endif
  for (auto tool : PhysicalToolIndex::all()) {
    active |= thermalManager.degTargetHotend(tool) != 0;
  }
  return active;
}

/**
 * Manage several activities:
 *  - Keep the command buffer full
 *  - Check for KILL button held down
 *  - Check for HOME button held down
 *  - Check if cooling fan needs to be switched on
 *  - Pulse FET_SAFETY_PIN if it exists
 */

void manage_inactivity() {

  if (queue.length < BUFSIZE) queue.get_available_commands();

  [[maybe_unused]] const millis_t ms = millis();

  #if HAS_KILL
    // #error dead code found by automatic analyses (see BFW-5461)

    // Check if the kill button was pressed and wait just in case it was an accidental
    // key kill key press
    // -------------------------------------------------------------------------------
    static int killCount = 0;   // make the inactivity button a bit less responsive
    const int KILL_DELAY = 750;
    if (!READ(KILL_PIN))
      killCount++;
    else if (killCount > 0)
      killCount--;

    // Exceeded threshold and we can confirm that it was not accidental
    // KILL the machine
    // ----------------------------------------------------------------
    if (killCount >= KILL_DELAY) {
      SERIAL_ERROR_MSG(MSG_KILL_BUTTON);
      kill();
    }
  #endif

  #if HAS_HOME
    // #error dead code found by automatic analyses (see BFW-5461)
    // Handle a standalone HOME button
    constexpr millis_t HOME_DEBOUNCE_DELAY = 1000UL;
    static millis_t next_home_key_ms; // = 0
    if (!IS_SD_PRINTING() && !READ(HOME_PIN)) { // HOME_PIN goes LOW when pressed
      const millis_t ms = millis();
      if (ELAPSED(ms, next_home_key_ms)) {
        next_home_key_ms = ms + HOME_DEBOUNCE_DELAY;
        LCD_MESSAGEPGM(MSG_AUTO_HOME);
        queue.enqueue_now_P(PSTR("G28"));
      }
    }
  #endif

  #if ENABLED(AUTO_POWER_CONTROL)
    powerManager.check();
  #endif

  #if ENABLED(MONITOR_DRIVER_STATUS)
    monitor_motor_drivers();
  #endif

  #if HAS_PLANNER()
    // Limit check_axes_activity frequency to 10Hz
    static millis_t next_check_axes_ms = 0;
    if (ELAPSED(ms, next_check_axes_ms)) {
      planner.check_axes_activity();
      next_check_axes_ms = ms + 100UL;
    }
  #else /* HAS_PLANNER() */
    // Apply fan speeds manually, because it is done in the planner.check_axes_activity()
    thermalManager.apply_print_fan_speed();
  #endif /* HAS_PLANNER() */

  #if PIN_EXISTS(FET_SAFETY)
    // #error dead code found by automatic analyses (see BFW-5461)
    static millis_t FET_next;
    if (ELAPSED(ms, FET_next)) {
      FET_next = ms + FET_SAFETY_DELAY;  // 2uS pulse every FET_SAFETY_DELAY mS
      OUT_WRITE(FET_SAFETY_PIN, !FET_SAFETY_INVERTED);
      DELAY_US(2);
      WRITE(FET_SAFETY_PIN, FET_SAFETY_INVERTED);
    }
  #endif
}

/**
 * Standard idle routine keeps the machine alive
 *
 * @param waiting
 *   @par @c true Caller is waiting for some event, release CPU to other tasks.
 *   @par @c false Caller has more data to process, do not release CPU.
 */
void idle(bool waiting) {
  #if HAS_PLANNER()
    endstops.event_handler();
  #endif

  #if ENABLED(EXTENSIBLE_UI)
    ExtUI::onIdle();
  #endif

  #if ENABLED(HOST_KEEPALIVE_FEATURE)
    gcode.host_keepalive();
  #endif

  manage_inactivity();

  thermalManager.manage_heater();

  #if HAS_AUTO_REPORTING
    if (!suspend_auto_report) {
      #if ENABLED(AUTO_REPORT_TEMPERATURES)
        thermalManager.auto_report_temperatures();
      #endif
    }
  #endif

  #if HAS_MMU2()
    MMU2::mmu2.mmu_loop();
  #endif

  #if HAS_PLANNER()
    PreciseStepping::loop();
  #endif /* HAS_PLANNER() */
  motor_driver_loop();

  #if ENABLED(NOZZLE_LOAD_CELL)
    if( EMotorStallDetector::Instance().Evaluate(stepper.axis_is_moving(E_AXIS), ! stepper.motor_direction(E_AXIS))){
        // E-motor stall has been detected, issue a modified M600
        SERIAL_ECHOLNPGM("E-motor stall detected");

        const auto virtual_tool = VirtualToolIndex::currently_selected_opt();
        if (!virtual_tool.has_value()) {
            bsod("EStall without a tool");
        }
        marlin_server::gcode_interrupt(GCodeLiteral{"M1601 V%.0f", (float) virtual_tool->to_raw()});
    }
  #endif

  if (waiting) delay(1);
}

/**
 * Marlin entry-point: Set up before the program loop
 *  - Set up the kill pin, filament runout, power hold
 *  - Start the serial port
 *  - Print startup messages and diagnostics
 *  - Get EEPROM or default settings
 *  - Initialize managers for:
 *    • temperature
 *    • planner
 *    • watchdog
 *    • stepper
 *    • photo pin
 *    • servos
 *    • LCD controller
 *    • Z probe sled
 *    • status LEDs
 */
void setup() {

  HAL_init();

  setup_killpin();

  #if HAS_TMC220x
    tmc_serial_begin();
  #endif

  setup_powerhold();

  #if HAS_STEPPER_RESET
    // #error dead code found by automatic analyses (see BFW-5461)
    disableStepperDrivers();
  #endif

  #if NUM_SERIAL > 0
    MYSERIAL0.begin(BAUDRATE);
    #if NUM_SERIAL > 1
      // #error dead code found by automatic analyses (see BFW-5461)
      MYSERIAL1.begin(BAUDRATE);
    #endif
  #endif

  #if NUM_SERIAL > 0
    uint32_t serial_connect_timeout = millis() + 1000UL;
    while (!MYSERIAL0 && PENDING(millis(), serial_connect_timeout)) { /*nada*/ }
    #if NUM_SERIAL > 1
      // #error dead code found by automatic analyses (see BFW-5461)
      serial_connect_timeout = millis() + 1000UL;
      while (!MYSERIAL1 && PENDING(millis(), serial_connect_timeout)) { /*nada*/ }
    #endif
  #endif

  SERIAL_ECHOLNPGM("start");
  SERIAL_ECHO_START();

  motor_serial_lock_init();
  motor_driver_init();
  motor_driver_check_connections();

  // Check startup - does nothing if bootloader sets MCUSR to 0
  byte mcu = HAL_get_reset_source();
  if (mcu &  1) SERIAL_ECHOLNPGM(MSG_POWERUP);
  if (mcu &  2) SERIAL_ECHOLNPGM(MSG_EXTERNAL_RESET);
  if (mcu &  4) SERIAL_ECHOLNPGM(MSG_BROWNOUT_RESET);
  if (mcu &  8) SERIAL_ECHOLNPGM(MSG_WATCHDOG_RESET);
  if (mcu & 32) SERIAL_ECHOLNPGM(MSG_SOFTWARE_RESET);
  HAL_clear_reset_source();

  SERIAL_ECHOPGM(MSG_MARLIN);
  SERIAL_CHAR(' ');
  SERIAL_ECHOLNPGM(SHORT_BUILD_VERSION);
  SERIAL_EOL();

  #if defined(STRING_DISTRIBUTION_DATE) && defined(STRING_CONFIG_H_AUTHOR)
    SERIAL_ECHO_MSG(
      MSG_CONFIGURATION_VER
      STRING_DISTRIBUTION_DATE
      MSG_AUTHOR STRING_CONFIG_H_AUTHOR
    );
    SERIAL_ECHO_MSG("Compiled: " __DATE__);
  #endif

  #if HAS_PLANNER()
    SERIAL_ECHO_START();
    SERIAL_ECHOLNPAIR(MSG_FREE_MEMORY, freeMemory(), MSG_PLANNER_BUFFER_BYTES, (int)sizeof(block_t) * (BLOCK_BUFFER_SIZE));
  #endif

  // UI must be initialized before EEPROM
  // (because EEPROM code calls the UI).

  #if ENABLED(EXTENSIBLE_UI)
    ui.reset_status();
  #endif

  // Load data from EEPROM if available (or use defaults)
  // This also updates variables in the planner, elsewhere
  settings.first_load();

  #if HAS_M206_COMMAND
    // Initialize current position based on home_offset
    current_position += home_offset;
  #endif

  #if HAS_PLANNER()
    // Vital to init stepper/planner equivalent for current_position
    sync_plan_position();
  #endif

  thermalManager.init();    // Initialize temperature loop

  print_job_timer.init();   // Initial setup of print job timer

  #if HAS_PLANNER()
    endstops.init();        // Init endstops and pullups

    // Init the motion system (order is relevant!)
    // NOTE: this enables (timer) interrupts!
    planner.init();
    stepper.init();
    PreciseStepping::init_stepper();
#if HAS_PHASE_STEPPING()
    phase_stepping::init();
#endif
    PreciseStepping::init();
    #ifdef ADVANCED_STEP_GENERATORS
      input_shaper::init();
      pressure_advance::init();
    #endif
  #endif /* HAS_PLANNER() */

  #if HAS_BED_PROBE
    endstops.enable_z_probe(false);
  #endif

  #if HAS_STEPPER_RESET
    // #error dead code found by automatic analyses (see BFW-5461)
    enableStepperDrivers();
  #endif

  #if HAS_HOME
    // #error dead code found by automatic analyses (see BFW-5461)
    SET_INPUT_PULLUP(HOME_PIN);
  #endif

  #ifdef Z_ALWAYS_ON
    enable_Z();
  #endif

  #if HAS_CASE_LIGHT
    // #error dead code found by automatic analyses (see BFW-5461)
      if (PWM_PIN(CASE_LIGHT_PIN)) SET_PWM(CASE_LIGHT_PIN); else SET_OUTPUT(CASE_LIGHT_PIN);
    update_case_light();
  #endif

  #if ENABLED(USE_WATCHDOG)
    watchdog_init();          // Reinit watchdog after HAL_get_reset_source call
  #endif

  #ifdef STARTUP_COMMANDS
    // #error dead code found by automatic analyses (see BFW-5461)
    queue.inject_P(PSTR(STARTUP_COMMANDS));
  #endif

  #if ENABLED(HOST_PROMPT_SUPPORT)
    host_action_prompt_end();
  #endif

  #if HAS_TEMP_HEATBREAK_CONTROL
    for (auto tool : PhysicalToolIndex::all()) {
      thermalManager.setTargetHeatbreak(DEFAULT_HEATBREAK_TEMPERATURE, tool);
    }
  #endif
}
