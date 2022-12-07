#pragma once
#include <stdint.h>
#include "cmsis_os.h"

namespace power_panic {

enum class AcFaultState : uint8_t {
    Inactive,
    Prepared,
    Triggered,
    Retracting,
    SaveState,
    WaitingToDie,
};

// TODO: internal state can be hidden by improving the interface
extern AcFaultState ac_fault_state;

// Return true if print state has been stored
bool state_stored();

/// Load the powerpanic state and setup print progress
/// @returns True if print can auto-recover
bool setup_auto_recover_check();

// Return the SFN media path of the print to be resumed
const char *stored_media_path();

// Clear existing stored state, reset internal state
void reset();

// Prepare the fixed data section in the flash, including MBL and filename data.
// Call once after MBL during the printing process (usually after a successful G29)
void prepare();

// Start resuming a stored print
void resume_print(bool start_paused);

// Actually resume the print after starting paused
void resume_continue();

// Main resume loop handler
void resume_loop();

// Main fault loop handler
void ac_fault_loop();

// AC fault ISR handler
void ac_fault_isr();

// AC fault Task handler
extern osThreadId ac_fault_task;

void ac_fault_main(void const *argument);

};
