#pragma once
#include <stdint.h>
#include "cmsis_os.h"
#include <atomic>

namespace power_panic {

// Return true if PowerPanic has been triggered and the panic_loop() should be called
bool panic_is_active();

// Main fault loop handler
void panic_loop();

// Return true if print state has been stored
bool state_stored();

// Return the SFN media path of the print to be resumed
const char *stored_media_path();

// Clear existing stored state, reset internal state
void reset();

// Prepare the fixed data section in the flash, including MBL and filename data.
// Call once after MBL during the printing process (usually after a successful G29)
void prepare();

// Start resuming a stored print
void resume_print();

// Actually resume the print after starting paused
void resume_continue();

// Main resume loop handler
void resume_loop();

bool is_power_panic_resuming();

// A power panic is triggered only in the event of an AC power failure in the print state
// ac_fault_triggered is set in all cases of AC power failure (it is used to disable EEPROM writing)
extern std::atomic_bool ac_fault_triggered;

/// Whether we should beep during the PP
extern std::atomic_bool should_beep;

// Current acFault pin status
bool is_ac_fault_active();

// Raise error if ac fault is present on startup, enable interrupt
void check_ac_fault_at_startup();

// AC fault ISR handler
void ac_fault_isr();

// AC fault Task handler
extern osThreadId ac_fault_task;

void ac_fault_task_main(void const *argument);

}; // namespace power_panic
