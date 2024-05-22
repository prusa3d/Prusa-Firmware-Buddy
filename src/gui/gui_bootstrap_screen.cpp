
#include <option/resources.h>
#include <option/has_puppies.h>
#include <option/has_puppies_bootloader.h>
#include <option/has_embedded_esp32.h>
#include <option/bootloader_update.h>
#include <option/bootloader.h>
#include "tasks.hpp"
#include "screen_splash.hpp"
#include "ScreenHandler.hpp"

osMutexDef(bootstrap_progress_lock);

// if this is build with bootloader, we take over when it drawn 50% of progress bar, so start with that
static constexpr unsigned STARTING_PERCENTAGE = option::bootloader ? 50 : 0;

// Sync structure for GUI bootstrap screen display
struct Progress {
    unsigned percent = STARTING_PERCENTAGE;
    const char *message = "";
    bool need_redraw = false;
    osMutexId lock_id = nullptr;

    Progress() {
        lock_id = osMutexCreate(osMutex(bootstrap_progress_lock));
    }
    ~Progress() {
        osMutexDelete(lock_id);
    }
};

static Progress *bootstrap_progress;

bool gui_bootstrap_screen_set_state(unsigned percent, const char *str) {
    assert(bootstrap_progress);

    osMutexWait(bootstrap_progress->lock_id, osWaitForever);

    // ignore if no change
    if (percent == bootstrap_progress->percent && str == bootstrap_progress->message) {
        osMutexRelease(bootstrap_progress->lock_id);
        return false;
    }

    bootstrap_progress->percent = percent;
    bootstrap_progress->message = str != nullptr ? str : "";
    bootstrap_progress->need_redraw = true;
    osMutexRelease(bootstrap_progress->lock_id);

    // wait a little to give GUI change to draw screen (it has lower priority, so we need to free up the CPU)
    osDelay(1);

    return true;
}

// Draw smooth progressbar 1s
// Bootstrap resets the progressbar so we go from 0%
static void fw_gui_splash_progress() {
    uint32_t start = gui::GetTick_ForceActualization();

    // take over at whatever was last %
    const uint8_t start_percent = bootstrap_progress->percent;
    const uint8_t percent_remaining = 100 - start_percent;

    uint32_t last_tick = 0;
    uint8_t percent = start_percent;
    while (percent != 100) {
        uint32_t tick = gui::GetTick_ForceActualization();

        if (last_tick != tick) {
            percent = start_percent + ((tick - start) * percent_remaining) / 1000;
            percent = std::min(uint8_t(100), percent);

            GUIStartupProgress progr = { unsigned(percent), std::nullopt };
            event_conversion_union un;
            un.pGUIStartupProgress = &progr;
            Screens::Access()->WindowEvent(GUI_event_t::GUI_STARTUP, un.pvoid);

            last_tick = tick;
            gui_redraw();
            osDelay(20);
        }
    }
}

void gui_bootstrap_screen_run() {
    bootstrap_progress = new Progress();

    assert(bootstrap_progress);
    // draw somehting on screen before bootstrap strart
    screen_splash_data_t::bootstrap_cb(bootstrap_progress->percent, bootstrap_progress->message);
    gui_redraw();
    // start flashing resources
    TaskDeps::provide(TaskDeps::Dependency::gui_screen_ready);

    while (TaskDeps::check(TaskDeps::Tasks::bootstrap_done) == false) {
        if (bootstrap_progress->need_redraw) {
            osMutexWait(bootstrap_progress->lock_id, osWaitForever);
            screen_splash_data_t::bootstrap_cb(bootstrap_progress->percent, bootstrap_progress->message);
            gui_redraw();
            bootstrap_progress->need_redraw = false;
            osMutexRelease(bootstrap_progress->lock_id);
        }
        // limit refresh rate to < 10FPS, thats more than enough for progressbar
        osDelay(100);
    }

    fw_gui_splash_progress(); // draw a smooth progressbar from last percent to 100%

    // now delete helper synchronization variable
    delete bootstrap_progress;
    bootstrap_progress = nullptr;
}
