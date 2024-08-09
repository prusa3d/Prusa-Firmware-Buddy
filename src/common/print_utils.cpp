/**
 * @file
 */
#include "print_utils.hpp"
#include "../Marlin/src/gcode/lcd/M73_PE.h"
#include "../lib/Marlin/Marlin/src/module/temperature.h"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "unique_file_ptr.hpp"
#include "timing.h"
#include "unistd.h"
#include "str_utils.hpp"
#include "tasks.hpp"
#include <usb_host.h>
#include <state/printer_state.hpp>
#include <transfers/transfer.hpp>
#include <feature/prusa/restore_z.h>
#include <gcode/gcode_reader_restore_info.hpp>

#include <option/bootloader.h>
#include <option/has_mmu2.h>

#if HAS_MMU2()
    #include <Marlin/src/feature/prusa/MMU2/mmu2_mk4.h>
#endif

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
    #if BOOTLOADER()
        #include "sys.h" // support for bootloader<1.2.3
    #endif
#endif

#if ENABLED(POWER_PANIC)
    #include "transfers/transfer.hpp"

static bool file_exists(const char *filename) {
    if (unique_file_ptr(fopen(filename, "r"))) {
        return true;
    }

    if (MutablePath path(filename); transfers::is_valid_transfer(path)) {
        return true;
    }

    return false;
}
#endif

/**
 * Restore Z coordinate after boot if enabled.
 * Restore print after power panic event.
 * Auto-start gcode.
 *
 * REBOOT_RESTORE_Z is hooked here just before power panic to
 * make sure it is not called later than power panic. If
 * restore_z::restore() would be called later than power panic it
 * might screw power panic restored Z coordinate.
 *
 * Z-coordinate restored by restore_z will be immediately rewritten by power panic.
 * So to make it clear it has no effect in case of power panic restore
 * it is explicitly skipped and cleared.
 *
 * Hooking REBOOT_RESTORE_Z here has also some down sides. This hook is
 * called only after USB storage is detected. This means REBOOT_RESTORE_Z
 * doesn't work if the USB flash drive is not connected in time. Also
 * it might take long to detect USB storage so some move can be initiated
 * by user or WUI and that movement may be screwed by restore_z.
 *
 * If this is the problem REBOOT_RESTORE_Z might be hooked much earlier.
 * E.g. Marlin.cpp setup().
 */
void run_once_after_boot() {
#if ENABLED(REBOOT_RESTORE_Z)
    #if ENABLED(POWER_PANIC)
    if (power_panic::state_stored()) {
        restore_z::clear();
    } else
    #endif
    {
        restore_z::restore();
    }
#else
    restore_z::clear();
#endif

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
                power_panic::resume_print();
                return;
            }
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

    if (!TaskDeps::check(TaskDeps::Dependency::usb_temp_gui_ready) && ticks_ms() >= current_time + rescan_delay) {
        current_time += rescan_delay;
        if (usb_host::is_media_inserted() && thermalManager.temperatures_ready() && TaskDeps::check(TaskDeps::Dependency::gui_ready)) {
            run_once_after_boot();
            TaskDeps::provide(TaskDeps::Dependency::usb_temp_gui_ready);
        } else if (current_time > max_rescan_time || !marlin_server::printer_idle()) {
            // no longer attempt to run the autostart sequence
            TaskDeps::provide(TaskDeps::Dependency::usb_temp_gui_ready);
        }
    }
}

void print_begin(const char *filename, marlin_server::PreviewSkipIfAble skip_preview) {
    marlin_client::print_start(filename, skip_preview);
    // FIXME: This should not be here and it should be handled
    // in Marlin. Needs refactoring!
    oProgressData.mInit();
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

bool is_tool_enabled(uint8_t tool) {
    if (tool >= EXTRUDERS) {
        return false;
    }

#if HAS_TOOLCHANGER()
    return prusa_toolchanger.getTool(tool).is_enabled();
#elif HAS_MMU2()
    return tool == 0 || MMU2::mmu2.Enabled();
#else
    return true;
#endif
}
