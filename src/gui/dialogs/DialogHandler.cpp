#include "DialogHandler.h"
#include "gui.h"

extern screen_t *pscreen_printing_serial;
void dialog_open_cb(dialog_t dialog, uint8_t data) {
    if (gui_get_nesting() > 1)
        return; //todo notify octoprint
    if (dialog == DLG_serial_printing) {
        screen_unloop(m876_blacklist, m876_blacklist_sz);

        if (screen_get_curr() != pscreen_printing_serial)
            screen_open(pscreen_printing_serial->id);
    }
}

void dialog_close_cb(dialog_t dialog) {
    if (gui_get_nesting() > 1)
        return; //todo notify octoprint
    if (dialog == DLG_serial_printing) {
        if (screen_get_curr() == pscreen_printing_serial)
            screen_close();
    }
}

void dialog_change_cb(dialog_t dialog, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    switch (dialog) {
    }
}
