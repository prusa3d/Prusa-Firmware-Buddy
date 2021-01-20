// window_dlg_load_unload.cpp

#include "window_dlg_load_unload.hpp"
#include "marlin_client.h"
#include "window_dlg_preheat.hpp"
#include "filament.h"
#include "DialogHandler.hpp"
#include "i18n.h"

dlg_result_t gui_dlg_load(string_view_utf8 caption) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    filament_t filament = gui_dlg_preheat(caption);
    if (filament == filament_t::NONE) {
        return dlg_result_t::aborted;
    }
    marlin_gcode_printf("M701 S\"%s\"", filaments[size_t(filament)].name);
    DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Load));
    return dlg_result_t::ok;
}

dlg_result_t gui_dlg_purge(string_view_utf8 caption) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    filament_t filament = gui_dlg_preheat_autoselect_if_able(caption);
    if (filament == filament_t::NONE)
        return dlg_result_t::aborted;

    marlin_gcode("M701 L0");
    DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Purge));
    return dlg_result_t::ok;
}

dlg_result_t gui_dlg_load_forced(void) {
    constexpr static const char *const label = N_("PREHEAT for LOAD");
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    filament_t filament = gui_dlg_preheat_forced(_(label));
    if (filament == filament_t::NONE) {
        return dlg_result_t::aborted; //dlg_result_t::aborted should not happen
    }
    marlin_gcode_printf("M701 S\"%s\"", filaments[size_t(filament)].name);
    DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Load));
    return dlg_result_t::ok;
}

dlg_result_t gui_dlg_unload(string_view_utf8 caption) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able(caption) == filament_t::NONE)
        return dlg_result_t::aborted; //user can choose "RETURN"
    marlin_gcode("M702");
    DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Unload));
    return dlg_result_t::ok;
}

dlg_result_t gui_dlg_unload_forced(void) {
    //todo must be called inside _gui_dlg, but nested dialogs are not supported now
    if (gui_dlg_preheat_autoselect_if_able_forced(_("PREHEAT for UNLOAD")) == filament_t::NONE)
        return dlg_result_t::aborted; //LD_ABORTED should not happen
    marlin_gcode("M702");
    DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Unload));
    return dlg_result_t::ok;
}
