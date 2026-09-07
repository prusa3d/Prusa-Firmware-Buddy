/**
 * @file
 */
#include "print_utils.hpp"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "../lib/Marlin/Marlin/src/module/temperature.h"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "path_utils.h"
#include "unique_file_ptr.hpp"
#include "timing.h"
#include "unistd.h"
#include "tasks.hpp"
#include <usb_host.h>
#include <state/printer_state.hpp>
#include <transfers/transfer.hpp>
#include <gcode/gcode_reader_restore_info.hpp>

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
#endif

#include <option/has_power_panic.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#if HAS_POWER_PANIC()
    #include "power_panic.hpp"
#endif

/**
 * Restore print after power panic event.
 * Auto-start gcode.
 */
void run_once_after_boot() {
#if HAS_POWER_PANIC()
    if (power_panic::state_stored()) {
        // Data has been saved: ensure we're coming either from self-reset (we reached the end of
        // the PP cycle due to a short power burst) OR brown-out has been detected. Clear the data
        // if the user pressed the reset button explicitly!
        bool reset_pp = !((HAL_RCC_CSR & (RCC_CSR_SFTRSTF | RCC_CSR_BORRSTF)));
        if (!reset_pp && transfers::is_valid_file_or_transfer(power_panic::stored_media_path()) && usb_host::is_media_inserted_since_startup()) {
            // load the panic data and setup print progress early
            // resume and bypass g-code autostart
            power_panic::resume_print();
            return;
        }
        if (reset_pp) {
            power_panic::reset();
        }
    }
#endif

    // g-code autostart
    static constexpr const char *autostart_filename = "/usb/AUTO.GCO";
    if (access(autostart_filename, F_OK) == 0) {
        // call directly marlin server start print. This function is not safe
        marlin_server::print_start(autostart_filename, GCodeReaderPosition(), marlin_server::PreviewSkipIfAble::all);
        oProgressData.mInit();
    }
}

void print_utils_loop() {
    static constexpr uint32_t rescan_delay = 1500; ///< Check run_once_after_boot this often [ms]
    static constexpr uint32_t max_rescan_time = 100000; ///< Wait for run_once_after_boot at most [ms]

    static uint32_t current_time = ticks_ms();

    if (!TaskDeps::check(TaskDeps::Dependency::autostart_done) && ticks_ms() >= current_time + rescan_delay) {
        current_time += rescan_delay;
        if (usb_host::is_media_inserted() && thermalManager.temperatures_ready() && TaskDeps::check(TaskDeps::Dependency::gui_ready)) {
            run_once_after_boot();
            TaskDeps::provide(TaskDeps::Dependency::autostart_done);
        } else if (current_time > max_rescan_time || !marlin_server::printer_idle()) {
            // no longer attempt to run the autostart sequence
            TaskDeps::provide(TaskDeps::Dependency::autostart_done);
        }
    }
}

DeleteResult remove_file(const char *path) {
    if (marlin_vars().media_SFN_path.equals(path)) {
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

    MutablePath mp(path);
    if (transfers::is_valid_transfer(mp)) {
        return DeleteResult::ActiveTransfer;
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

uint8_t get_num_of_enabled_tools() {
#if HAS_TOOLCHANGER()
    return prusa_toolchanger.get_num_enabled_tools();
#elif HAS_MMU2()
    return MMU2::mmu2.Enabled() ? EXTRUDERS : 1; // MMU has all slots available
#else
    return EXTRUDERS;
#endif
}
