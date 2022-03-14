/**
 * @file MItem_filament.cpp
 */

#include "MItem_filament.hpp"
#include "sound.hpp"
#include "DialogHandler.hpp"
#include "window_dlg_load_unload.hpp"

/*****************************************************************************/
//MI_LOAD
MI_LOAD::MI_LOAD()
    : MI_event_dispatcher(_(label)) {}

void MI_LOAD::Do() {
    if ((Filaments::CurrentIndex() == filament_t::NONE) || (MsgBoxWarning(_(warning_loaded), Responses_YesNo, 1) == Response::Yes)) {
        PreheatStatus::Dialog(PreheatMode::Load, RetAndCool_t::Return);
    }
}

/*****************************************************************************/
//MI_UNLOAD
MI_UNLOAD::MI_UNLOAD()
    : MI_event_dispatcher(_(label)) {}

void MI_UNLOAD::Do() {
    PreheatStatus::Dialog(PreheatMode::Unload, RetAndCool_t::Return);
    Sound_Stop(); // TODO what is Sound_Stop(); doing here?
}

/*****************************************************************************/
//MI_CHANGE
MI_CHANGE::MI_CHANGE()
    : MI_event_dispatcher(_(label)) {}

void MI_CHANGE::Do() {
    PreheatStatus::Dialog(PreheatMode::Change_phase1, RetAndCool_t::Return);
    Sound_Stop(); // TODO what is Sound_Stop(); doing here?
}

/*****************************************************************************/
//MI_PURGE
MI_PURGE::MI_PURGE()
    : MI_event_dispatcher(_(label)) {}

void MI_PURGE::Do() {
    PreheatStatus::Dialog(PreheatMode::Purge, RetAndCool_t::Return);
}
