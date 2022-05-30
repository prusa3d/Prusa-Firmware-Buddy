/**
 * @file MItem_filament.cpp
 */

#include "MItem_filament.hpp"
#include "sound.hpp"
#include "DialogHandler.hpp"
#include "window_dlg_load_unload.hpp"
#include "marlin_client.h"

/*****************************************************************************/
//MI_LOAD
MI_LOAD::MI_LOAD()
    : MI_event_dispatcher(_(label)) {}

void MI_LOAD::Do() {
    if ((Filaments::CurrentIndex() == filament_t::NONE) || (MsgBoxWarning(_(warning_loaded), Responses_YesNo, 1) == Response::Yes)) {
        marlin_gcode("M701 W2"); // load with return option
    }
}

/*****************************************************************************/
//MI_UNLOAD
MI_UNLOAD::MI_UNLOAD()
    : MI_event_dispatcher(_(label)) {}

void MI_UNLOAD::Do() {
    marlin_gcode("M702 W2"); // unload with return option
    Sound_Stop();            // TODO what is Sound_Stop(); doing here?
}

/*****************************************************************************/
//MI_CHANGE
MI_CHANGE::MI_CHANGE()
    : MI_event_dispatcher(_(label)) {}

void MI_CHANGE::Do() {
    marlin_gcode("M1600"); // non print filament change
    Sound_Stop();          // TODO what is Sound_Stop(); doing here?
}

/*****************************************************************************/
//MI_PURGE
MI_PURGE::MI_PURGE()
    : MI_event_dispatcher(_(label)) {}

void MI_PURGE::Do() {
    marlin_gcode("M701 L0 W2"); // load with distance 0 and return option
}

/*****************************************************************************/
//MI_COOLDOWN
MI_COOLDOWN::MI_COOLDOWN()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}

void MI_COOLDOWN::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}
