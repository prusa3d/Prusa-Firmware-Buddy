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
#include "window_frame.hpp"
#include <limits>

#include "ScreenHandler.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, MI_RETURN,
    MI_Filament<filament_t::PLA>,
    MI_Filament<filament_t::PETG>,
    MI_Filament<filament_t::ASA>,
    MI_Filament<filament_t::ABS>,
    MI_Filament<filament_t::PC>,
    MI_Filament<filament_t::FLEX>,
    MI_Filament<filament_t::HIPS>,
    MI_Filament<filament_t::PP>,
    MI_Filament<filament_t::NONE>>;

// is used in firstlay calibration and print preview, does not have return and cooldown
using ScreenNoRet = ScreenMenu<EHeader::Off, EFooter::On,
    MI_Filament<filament_t::PLA>,
    MI_Filament<filament_t::PETG>,
    MI_Filament<filament_t::ASA>,
    MI_Filament<filament_t::ABS>,
    MI_Filament<filament_t::PC>,
    MI_Filament<filament_t::FLEX>,
    MI_Filament<filament_t::HIPS>,
    MI_Filament<filament_t::PP>>;

template <class T>
filament_t make_preheat_dialog(string_view_utf8 caption) {
    set_last_preheated_filament(filament_t::NONE);
    T dlg(caption, Screens::Access()->Get());
    create_blocking_dialog_from_normal_window(dlg);
    return get_last_preheated_filament();
}

filament_t gui_dlg_preheat(string_view_utf8 caption) {
    return make_preheat_dialog<Screen>(caption);
}

filament_t gui_dlg_preheat_autoselect_if_able(string_view_utf8 caption) {
    const filament_t fil = get_filament();
    if (fil == filament_t::NONE) {
        //no filament selected
        return gui_dlg_preheat(caption);
    } else {
        //when filament is known, but heating is off, just turn it on and do not ask
        marlin_vars_t *p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
        if (p_vars->target_nozzle != filaments[size_t(fil)].nozzle) {
            marlin_gcode_printf("M104 S%d", (int)filaments[size_t(fil)].nozzle);
            marlin_gcode_printf("M140 S%d", (int)filaments[size_t(fil)].heatbed);
        }
    }
    return fil;
}

//no return option
filament_t gui_dlg_preheat_forced(string_view_utf8 caption) {
    return make_preheat_dialog<ScreenNoRet>(caption);
}

//no return option
filament_t gui_dlg_preheat_autoselect_if_able_forced(string_view_utf8 caption) {
    const filament_t fil = get_filament();
    if (fil == filament_t::NONE) {
        //no filament selected
        return gui_dlg_preheat_forced(caption);
    } else {
        //when filament is known, but heating is off, just turn it on and do not ask
        marlin_vars_t *p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
        if (p_vars->target_nozzle != filaments[size_t(fil)].nozzle) {
            marlin_gcode_printf("M104 S%d", (int)filaments[size_t(fil)].nozzle);
            marlin_gcode_printf("M140 S%d", (int)filaments[size_t(fil)].heatbed);
        }
    }
    return fil;
}

/*****************************************************************************/
//DialogMenuPreheat::I_MI_Filament
void DialogMenuPreheat::I_MI_Filament::click_at(filament_t filament_index) {
    const Response response = filaments[size_t(filament_index)].response;
    marlin_FSM_response(PhasesPreheat::UserTempSelection, response);
}

/*****************************************************************************/
//DialogMenuPreheat::MI_RETURN
DialogMenuPreheat::MI_RETURN::MI_RETURN()
    : WI_LABEL_t(_(label), IDR_PNG_folder_up_16px, is_enabled_t::yes, is_hidden_t::no) {
}

void DialogMenuPreheat::MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    marlin_FSM_response(PhasesPreheat::UserTempSelection, Response::Abort);
}

/*****************************************************************************/
//DialogMenuPreheat

DialogMenuPreheat::DialogMenuPreheat(string_view_utf8 name)
    : AddSuperWindow<IDialogMarlin>(GuiDefaults::RectScreenNoFoot)
    , menu(this, GuiDefaults::RectScreenBody, &container)
    , header(this) {
    header.SetText(name);
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);
}

bool DialogMenuPreheat::change(uint8_t phs, uint8_t progress_tot, uint8_t progress) {
    return true;
}
