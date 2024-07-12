/*
 * window_dlg_preheat.cpp
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#include "window_dlg_preheat.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "stdlib.h"
#include "i18n.h"
#include <limits>

constexpr static const char *load_preheat = N_("Preheating for load");
constexpr static const char *unload_preheat = N_("Preheating for unload");
constexpr static const char *purge_preheat = N_("Preheating for purge");
constexpr static const char *index_error = "Index error"; // intentionally not to be translated

/*****************************************************************************/
// NsPreheat::I_MI_Filament
NsPreheat::I_MI_Filament::I_MI_Filament(FilamentType filament_type)
    : WiInfo<info_len>({}, nullptr, is_enabled_t::yes, is_hidden_t::no)
    , filament_params(filament::get_description(filament_type))
    , filament_type(filament_type) //
{
    SetLabel(string_view_utf8::MakeRAM(filament_params.name));

    char buff[info_len];
    snprintf(buff, sizeof(buff), filament_params.heatbed_temperature > 100 ? "%3u/%3u " : "%3u/%2u  ", filament_params.nozzle_temperature, filament_params.heatbed_temperature); // extra space(s) at the end are intended .. "260/100 " or  "215/60  "
    ChangeInformation(buff);
}

void NsPreheat::I_MI_Filament::click(IWindowMenu &) {
    marlin_client::FSM_response_variant(PhasesPreheat::UserTempSelection, FSMResponseVariant::make<FilamentType>(filament_type));
}

/*****************************************************************************/
// NsPreheat::MI_RETURN
NsPreheat::MI_RETURN::MI_RETURN()
    : IWindowMenuItem(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
    has_return_behavior_ = true;
}

void NsPreheat::MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, Response::Abort);
}

/*****************************************************************************/
// NsPreheat::MI_COOLDOWN
NsPreheat::MI_COOLDOWN::MI_COOLDOWN()
    : IWindowMenuItem(_(get_response_text(Response::Cooldown)), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void NsPreheat::MI_COOLDOWN::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, Response::Cooldown);
}

/*****************************************************************************/
// DialogMenuPreheat
DialogMenuPreheat::DialogMenuPreheat(fsm::BaseData data)
    : IDialogMarlin(get_title(data).isNULLSTR() ? GuiDefaults::RectScreenNoHeader : GuiDefaults::RectScreen)
    , menu(this, GuiDefaults::RectScreenNoHeader, &menu_container)
    , header(this) { // header registration should fail in case name.isNULLSTR(), it is OK
    string_view_utf8 title = get_title(data);
    title.isNULLSTR() ? header.Hide() : header.SetText(title); // hide it anyway, to be safe

    CaptureNormalWindow(menu);

    const PreheatData preheat_data = get_type(data);

    NsPreheat::MI_RETURN &menu_item_return = menu_container.Item<NsPreheat::MI_RETURN>();
    if (preheat_data.HasReturnOption()) {
        menu_item_return.show();
    } else {
        menu_item_return.hide();
    }

    NsPreheat::MI_COOLDOWN &menu_item_cooldown = menu_container.Item<NsPreheat::MI_COOLDOWN>();
    if (preheat_data.HasCooldownOption()) {
        menu_item_cooldown.show();
    } else {
        menu_item_cooldown.hide();
    }
}

PreheatData DialogMenuPreheat::get_type(fsm::BaseData data) {
    return PreheatData(data.GetData());
}

string_view_utf8 DialogMenuPreheat::get_title(fsm::BaseData data) {
    switch (get_type(data).Mode()) {
    case PreheatMode::None:
        return string_view_utf8::MakeNULLSTR();
    case PreheatMode::Load:
    case PreheatMode::Autoload:
        return _(load_preheat);
    case PreheatMode::Unload:
        return _(unload_preheat);
    case PreheatMode::Purge:
        return _(purge_preheat);
    case PreheatMode::Change_phase1:
        return _(unload_preheat); // use unload caption, not a bug
    case PreheatMode::Change_phase2:
        return _(load_preheat); // use load caption, not a bug
    default:
        break;
    }
    return string_view_utf8::MakeCPUFLASH((const uint8_t *)index_error);
}
