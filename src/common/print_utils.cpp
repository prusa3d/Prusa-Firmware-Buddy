#include "print_utils.hpp"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "../lib/Marlin/Marlin/src/module/temperature.h"
#include "marlin_client.hpp"
#include "media.h"
#include "marlin_server.hpp"
#include "timing.h"
#include "config.h"    // GUI_WINDOW_SUPPORT
#include "guiconfig.h" // GUI_WINDOW_SUPPORT
#include "unistd.h"
#include "tasks.hpp"
#include <state/printer_state.hpp>

#include <option/bootloader.h>

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
    #if BOOTLOADER()
        #include "sys.h" // support for bootloader<1.2.3
    #endif
#endif

static const char *autostart_filename = "/usb/AUTO.GCO";
static bool run_once_done = false;
static uint32_t current_time = 0;
static uint32_t rescan_delay = 1500;
static uint32_t max_rescan_time = 100000;

#if ENABLED(POWER_PANIC)
static bool file_exists(const char *filename) {
    FILE *open_file = fopen(filename, "r");
    bool file_exists = open_file != nullptr;
    if (open_file)
        fclose(open_file);
    return file_exists;
}
#endif

void run_once_after_boot() {
#if ENABLED(POWER_PANIC)
    if (power_panic::state_stored()) {
        // Data has been saved: ensure we're coming either from self-reset (we reached the end of
        // the PP cycle due to a short power burst) OR brown-out has been detected. Clear the data
        // if the user pressed the reset button explicitly!
        bool reset_pp = !((HAL_RCC_CSR & (RCC_CSR_SFTRSTF | RCC_CSR_BORRSTF)));
    #if BOOTLOADER()
        if (version_less_than(&boot_version, 1, 2, 3)) {
            // bootloader<1.2.3 clears the RCC_CSR register, so ignore reset flags completely.
            // TODO: remove this compatibility hack for the final MK4 release
            reset_pp = false;
        }
    #endif
        if (!reset_pp) {
            // load the panic data and setup print progress early
            bool auto_recover = power_panic::setup_auto_recover_check();
            const char *path = power_panic::stored_media_path();
            bool resume = false;
            bool path_exists = file_exists(path);
            if (path_exists) {
                resume = true;
            } else if (!path_exists) {
                // TODO: ask about wrong stick. do not clear the state yet!
                reset_pp = false;
            }
            if (resume) {
                // resume and bypass g-code autostart
                power_panic::resume_print(auto_recover);
                TaskDeps::provide(TaskDeps::Dependency::power_panic_initialized);
                return;
            }
        }
        if (reset_pp)
            power_panic::reset();
    }
#endif

    // g-code autostart
    if (access(autostart_filename, F_OK) == 0) {
        // call directly marlin server start print. This function is not safe
        marlin_server::print_start(autostart_filename, true);
        oProgressData.mInit();
    }

    TaskDeps::provide(TaskDeps::Dependency::power_panic_initialized);
}

void print_utils_loop() {
    if (run_once_done == false && HAL_GetTick() >= current_time + rescan_delay) {
        current_time += rescan_delay;
        if (media_get_state() == media_state_INSERTED && thermalManager.temperatures_ready()) {
            run_once_done = true;
            run_once_after_boot();
        } else if (current_time > max_rescan_time || !marlin_server::printer_idle()) {
            // no longer attempt to run the autostart sequence
            run_once_done = true;
        }
    }
}

void print_begin(const char *filename, bool skip_preview) {
    marlin_client::print_start(filename, skip_preview);
    // FIXME: This should not be here and it should be handled
    // in Marlin. Needs refactoring!
    oProgressData.mInit();
}

DeleteResult remove_file(const char *path) {
    if (marlin_vars()->media_SFN_path.equals(path)) {
        switch (printer_state::get_state()) {
        case printer_state::DeviceState::Finished:
        case printer_state::DeviceState::Stopped:
            // If the state is Finished or Stopped, this can't fail, the only reason of
            // failure would be a change in the state between the check above
            // and this call.
            marlin_client::print_exit();
            if (!marlin_client::is_print_exited()) {
                return DeleteResult::Busy;
            }
            break;
        case printer_state::DeviceState::Paused:
        case printer_state::DeviceState::Printing:
            return DeleteResult::Busy;
        default:
            break;
        }
    }
    int result = remove(path);
    if (result == -1) {
        if (errno == EBUSY) {
            return DeleteResult::Busy;
        } else {
            return DeleteResult::GeneralError;
        }
    }
    return DeleteResult::Success;
}
