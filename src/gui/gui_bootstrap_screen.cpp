
#include <option/resources.h>
#include <option/has_puppies.h>
#include <option/has_puppies_bootloader.h>
#include <option/has_embedded_esp32.h>
#include <option/bootloader_update.h>
#include <option/bootloader.h>
#include "tasks.hpp"
#include "screen_splash.hpp"

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

void gui_bootstrap_screen_set_state(unsigned percent, const char *str) {
    assert(bootstrap_progress);

    osMutexWait(bootstrap_progress->lock_id, osWaitForever);

    // ignore if no change
    if (percent == bootstrap_progress->percent && str == bootstrap_progress->message) {
        osMutexRelease(bootstrap_progress->lock_id);
        return;
    }

    bootstrap_progress->percent = percent;
    bootstrap_progress->message = str != nullptr ? str : "";
    bootstrap_progress->need_redraw = true;
    osMutexRelease(bootstrap_progress->lock_id);

    // wait a little to give GUI change to draw screen (it has lower priority, so we need to free up the CPU)
    osDelay(1);
}

void gui_bootstrap_screen_init() {
    bootstrap_progress = new Progress();
    assert(bootstrap_progress);
}

void gui_bootstrap_screen_run() {
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
}

unsigned gui_bootstrap_screen_get_percent() {
    assert(bootstrap_progress);
    return bootstrap_progress->percent;
}

void gui_bootstrap_screen_delete() {
    // now delete helper synchronization variable
    delete bootstrap_progress;
}
