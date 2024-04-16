#include "puppies/puppy_task.hpp"

#include "task.h"

#include <iterator>
#include "cmsis_os.h"
#include "log.h"
#include "main.h"
#include "puppies/modular_bed.hpp"
#include "puppies/Dwarf.hpp"
#include "puppies/PuppyBootstrap.hpp"
#include "timing.h"
#include "Marlin/src/module/stepper.h"
#include "Marlin/src/module/prusa/toolchanger.h"
#include <option/has_embedded_esp32.h>
#if HAS_EMBEDDED_ESP32()
    #include <esp_flash.hpp>
#endif
#include <tasks.hpp>
#include <option/has_dwarf.h>
#include <ccm_thread.hpp>
#include "bsod.h"
#include "gui_bootstrap_screen.hpp"

LOG_COMPONENT_DEF(Puppies, LOG_SEVERITY_DEBUG);

namespace buddy::puppies {

osThreadId puppy_task_handle;

std::atomic<bool> stop_request = false; // when this is set to true, puppy task will gracefully stop its execution

static PuppyBootstrap::BootstrapResult bootstrap_puppies(PuppyBootstrap::BootstrapResult minimal_config, bool first_run) {
    // boostrap first
    log_info(Puppies, "Starting bootstrap");
    PuppyBootstrap puppy_bootstrap(PuppyModbus::share_buffer(), [first_run](PuppyBootstrap::Progress progress) {
        bool log = true;
        if (first_run) {
            // report progress to gui bootstrap screen only if first run - if this is puppy recoverery, there is no bootstrap screen anymore
            log = gui_bootstrap_screen_set_state(progress.percent_done, progress.description());
        }
        if (log) {
            log_info(Puppies, "Bootstrap stage: %s, percent %d", progress.description(), progress.percent_done);
        }
    });
    return puppy_bootstrap.run(minimal_config);
}

static void verify_puppies_running() {
    // wait for all the puppies to be reacheable
    log_info(Puppies, "Waiting for puppies to boot");

    auto err_supressor = PuppyModbus::ErrorLogSupressor();

    constexpr uint32_t WAIT_TIME = 5000;
    auto reacheability_wait_start = ticks_ms();
    do {
        bool modular_bed_ok = !modular_bed.is_enabled() || (modular_bed.ping() != CommunicationStatus::ERROR);

        uint8_t num_dwarfs_ok = 0, num_dwarfs_dead = 0;
#if HAS_DWARF()
        for (Dwarf &dwarf : dwarfs) {
            if (dwarf.is_enabled()) {
                if (dwarf.ping() != CommunicationStatus::ERROR) {
                    ++num_dwarfs_ok;
                } else {
                    ++num_dwarfs_dead;
                }
            }
        }
#endif

        if (num_dwarfs_dead == 0 && modular_bed_ok) {
            log_info(Puppies, "All puppies are reacheable. Continuing");
            return;
        } else if (ticks_diff(reacheability_wait_start + WAIT_TIME, ticks_ms()) > 0) {
            log_info(Puppies, "Puppies not ready (dwarfs_num: %d/%d, bed: %i), waiting another 200 ms",
                num_dwarfs_ok, num_dwarfs_ok + num_dwarfs_dead, modular_bed_ok);
            osDelay(200);
            continue;
        } else {
            fatal_error(ErrCode::ERR_SYSTEM_PUPPY_RUN_TIMEOUT);
        }
    } while (true);
}

static void puppy_task_loop() {
#if ENABLED(PRUSA_TOOLCHANGER)
    size_t slow_stage = 0; ///< Switch slow action
#endif

    // periodically update puppies until there is a failure
    while (true) {
        if (stop_request) {
            return;
        }

        [[maybe_unused]] uint32_t cycle_ticks = ticks_ms(); ///< Only one tick read per cycle, value will be reused by last_ticks_ms()
        // One slow action
        bool worked = false;
#if ENABLED(PRUSA_TOOLCHANGER)
        if (!prusa_toolchanger.update()) {
            return;
        }

        // Get dwarf that is selected
        // The source variable is set in this thread in prusa_toolchanger.update() called above, so no race
        Dwarf &active = prusa_toolchanger.getActiveToolOrFirst(); ///< Currently selected dwarf

        // Fast fifo pull from selected dwarf
        if (active.is_selected()) {
            bool more = true; ///< Pull while there is something in fifo
            // Pull fifo only this many times
            for (int active_fifo_attempts = 5; more && active_fifo_attempts > 0; active_fifo_attempts--) {
                if (active.pull_fifo(more) == CommunicationStatus::ERROR) {
                    return;
                }
            }
        } else {
            osDelay(1); // No dwarf is selected, wait a bit
        }

        size_t orig_stage = slow_stage;
        do {
            // Increment stage, there are 2 actions per dwarf and one modular bed
            if (++slow_stage >= (2 * std::size(dwarfs) + 1)) {
                slow_stage = 0;
            }

            if (slow_stage / 2 < std::size(dwarfs)) { // Two actions per dwarf
                Dwarf &dwarf = dwarfs[slow_stage / 2];
                if (!dwarf.is_enabled()) {
                    continue; // skip if this dwarf is not enabled
                }

                if (slow_stage % 2) {
                    if (&active == &dwarf) {
                        continue; // Skip selected dwarf
                    }

                    // Fast refresh of non-selected dwarf
                    CommunicationStatus status = dwarf.fifo_refresh(cycle_ticks);
                    if (status == CommunicationStatus::ERROR) {
                        return;
                    }
                    worked |= status == CommunicationStatus::OK;
                } else {
                    // Slow refresh of non-selected dwarf
                    CommunicationStatus status = dwarf.refresh();
                    if (status == CommunicationStatus::ERROR) {
                        return;
                    }
                    worked |= status == CommunicationStatus::OK;
                }
            } else
#endif
            {
                // Try slow refresh of modular bed
                if (modular_bed.refresh() == CommunicationStatus::ERROR) {
                    return;
                }
            }
#if ENABLED(PRUSA_TOOLCHANGER)
        } while (!worked && slow_stage != orig_stage); // End if we did some work or if no stage has anything to do
#endif
        osDelay(worked ? 1 : 2); // Longer delay if we did no work
    }
}

static bool puppy_initial_scan() {
    // init each puppy
#if HAS_DWARF()
    for (Dwarf &dwarf : dwarfs) {
        if (dwarf.is_enabled()) {
            if (dwarf.initial_scan() == CommunicationStatus::ERROR) {
                return false;
            }
        }
    }
#endif

    if (modular_bed.initial_scan() == CommunicationStatus::ERROR) {
        return false;
    }
    return true;
}

static void puppy_task_body([[maybe_unused]] void const *argument) {

#if BOARD_VER_HIGHER_OR_EQUAL_TO(0, 5, 0)
    // This is temporary, remove once everyone has compatible hardware.
    // Requires new sandwich rev. 06 or rev. 05 with R83 removed.

    #if HAS_EMBEDDED_ESP32()
    // Power on the ESP
    hw::espPower.write(hw::Pin::State::high);

    // Flash ESP
    ESPFlash esp_flash;
    auto esp_result = esp_flash.flash();
    if (esp_result != ESPFlash::State::Done) {
        log_error(Puppies, "ESP flash failed with %u", static_cast<unsigned>(esp_result));
        ESPFlash::fatal_err(esp_result);
    }
    TaskDeps::provide(TaskDeps::Dependency::esp_flashed);
    #endif
#endif

    bool first_run = true;

    // by default, we want one modular bed and one dwarf
    PuppyBootstrap::BootstrapResult minimal_puppy_config = PuppyBootstrap::MINIMAL_PUPPY_CONFIG;

    do {
        // reset and flash the puppies
        auto bootstrap_result = bootstrap_puppies(minimal_puppy_config, first_run);
        // once some puppies are detected, consider this minimal puppy config (do no allow disconnection of puppy while running)
        minimal_puppy_config = bootstrap_result;

        // set what puppies are connected
        modular_bed.set_enabled(bootstrap_result.is_dock_occupied(Dock::MODULAR_BED));
#if HAS_DWARF()
        for (int i = 0; i < DWARF_MAX_COUNT; i++) {
            dwarfs[i].set_enabled(bootstrap_result.is_dock_occupied(Dock::DWARF_1 + i));
        }
#endif

        // wait for puppies to boot up, ensure they are running
        verify_puppies_running();

        do {
            // do intial scan of puppies to init them
            if (!puppy_initial_scan()) {
                break;
            }

#if ENABLED(PRUSA_TOOLCHANGER)
            // select active tool (previously active tool, or first one when starting)
            if (!prusa_toolchanger.init(first_run)) {
                log_error(Puppies, "Unable to select tool, retrying");
                break;
            }
#endif

            TaskDeps::provide(TaskDeps::Dependency::puppies_ready);
            first_run = false;
            log_info(Puppies, "Puppies are ready");

            TaskDeps::wait(TaskDeps::Tasks::puppy_run);

            // write current Marlin's state of the E TMC
            stepperE0.push();

            // now run puppy main loop
            puppy_task_loop();
        } while (false);

        if (stop_request) {
            // stop of puppy task was requested, stop here gracefully, without holding any mutexes and such
            osThreadSuspend(nullptr);
        }

        log_error(Puppies, "Communication error, going to recovery puppies");
        osDelay(1300); // Needs to be here to give puppies time to finish dumping
    } while (true);
}

void start_puppy_task() {

    osThreadDef(puppies, puppy_task_body, TASK_PRIORITY_PUPPY_TASK, 0, 896);
    puppy_task_handle = osThreadCreate(osThread(puppies), NULL);
}

void suspend_puppy_task() {
    // ask puppy thread to stop its execution
    stop_request = true;
}

} // namespace buddy::puppies
