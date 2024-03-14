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

constexpr static const char load_preheat[] = N_("Preheating for load");
constexpr static const char unload_preheat[] = N_("Preheating for unload");
constexpr static const char purge_preheat[] = N_("Preheating for purge");
constexpr static const char index_error[] = "Index error"; // intentionally not to be translated

/*****************************************************************************/
// NsPreheat::I_MI_Filament
NsPreheat::I_MI_Filament::I_MI_Filament(string_view_utf8 name, unsigned t_noz, unsigned t_bed)
    : WiInfo<info_len>(name, nullptr, is_enabled_t::yes, is_hidden_t::no, ExtensionLikeLabel::yes) {
    char buff[info_len];
    snprintf(buff, sizeof(buff), t_bed > 100 ? "%3u/%3u " : "%3u/%2u  ", t_noz, t_bed); // extra space(s) at the end are intended .. "260/100 " or  "215/60  "
    ChangeInformation(buff);
}

void NsPreheat::I_MI_Filament::click_at(filament::Type filament) {
    const Response response = filament::get_description(filament).response;
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, response);
}

/*****************************************************************************/
// NsPreheat::MI_RETURN
NsPreheat::MI_RETURN::MI_RETURN()
    : WI_LABEL_t(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void NsPreheat::MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, Response::Abort);
}

/*****************************************************************************/
// NsPreheat::MI_COOLDOWN
NsPreheat::MI_COOLDOWN::MI_COOLDOWN()
    : WI_LABEL_t(_(BtnResponse::GetText(Response::Cooldown)), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void NsPreheat::MI_COOLDOWN::click([[maybe_unused]] IWindowMenu &window_menu) {
    const Response response = filament::get_description(filament::Type::NONE).response;
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, response);
}

/*****************************************************************************/
// DialogMenuPreheat
DialogMenuPreheat::DialogMenuPreheat(fsm::BaseData data)
    : AddSuperWindow<IDialogMarlin>(get_title(data).isNULLSTR() ? GuiDefaults::RectScreenNoHeader : GuiDefaults::RectScreen)
    , menu(this, GuiDefaults::RectScreenNoHeader, newContainer(get_type(data)))
    , header(this) { // header registration should fail in case name.isNULLSTR(), it is OK
    string_view_utf8 title = get_title(data);
    title.isNULLSTR() ? header.Hide() : header.SetText(title); // hide it anyway, to be safe

    CaptureNormalWindow(menu);
}

IWinMenuContainer *DialogMenuPreheat::newContainer(PreheatData type) {
    switch (type.RetAndCool()) {
    case RetAndCool_t::Both:
        return new (&container_mem_space) NsPreheat::MenuContainerHasRetCool;
    case RetAndCool_t::Return:
        return new (&container_mem_space) NsPreheat::MenuContainerHasRet;
    case RetAndCool_t::Cooldown:
        return new (&container_mem_space) NsPreheat::MenuContainerHasCool;
    case RetAndCool_t::Neither:
    default:
        break;
    }
    return new (&container_mem_space) NsPreheat::MenuContainer;
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
