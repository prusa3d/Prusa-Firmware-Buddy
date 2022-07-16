/*
 * window_dlg_preheat.cpp
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#include "window_dlg_preheat.hpp"
#include "marlin_client.h"
#include "resource.h"
#include "stdlib.h"
#include "i18n.h"
#include <limits>

/*****************************************************************************/
//NsPreheat::I_MI_Filament
void NsPreheat::I_MI_Filament::click_at(filament_t filament_index) {
    const Response response = Filaments::Get(filament_index).response;
    marlin_FSM_response(PhasesPreheat::UserTempSelection, response);
}

/*****************************************************************************/
//NsPreheat::MI_RETURN
NsPreheat::MI_RETURN::MI_RETURN()
    : WI_LABEL_t(_(label), IDR_PNG_folder_up_16px, is_enabled_t::yes, is_hidden_t::no) {
}

void NsPreheat::MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    marlin_FSM_response(PhasesPreheat::UserTempSelection, Response::Abort);
}

/*****************************************************************************/
//DialogMenuPreheat
DialogMenuPreheat::DialogMenuPreheat(string_view_utf8 name, PreheatData type)
    : AddSuperWindow<IDialogMarlin>(name.isNULLSTR() ? GuiDefaults::RectScreenNoHeader : GuiDefaults::RectScreen)
    , menu(this, GuiDefaults::RectScreenNoHeader, newContainer(type))
    , header(this) {                                         // header registration should fail in case name.isNULLSTR(), it is OK
    name.isNULLSTR() ? header.Hide() : header.SetText(name); // hide it anyway, to be safe

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

bool DialogMenuPreheat::change(uint8_t phs, fsm::PhaseData data) {
    return true;
}
