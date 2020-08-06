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
#include "../lang/i18n.h"
#include "window_frame.hpp"
#include <limits>
#include "MItem_tools.hpp"
#include "screen_menu.hpp"
#include "ScreenHandler.hpp"
#include "IDialog.hpp"

using Screen = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None, MI_RETURN,
    MI_Filament<FILAMENT_PLA>,
    MI_Filament<FILAMENT_PETG>,
    MI_Filament<FILAMENT_ASA>,
    MI_Filament<FILAMENT_ABS>,
    MI_Filament<FILAMENT_PC>,
    MI_Filament<FILAMENT_FLEX>,
    MI_Filament<FILAMENT_HIPS>,
    MI_Filament<FILAMENT_PP>,
    MI_Filament<FILAMENT_NONE>>;

using ScreenNoRet = ScreenMenu<EHeader::Off, EFooter::On, HelpLines_None,
    MI_Filament<FILAMENT_PLA>,
    MI_Filament<FILAMENT_PETG>,
    MI_Filament<FILAMENT_ASA>,
    MI_Filament<FILAMENT_ABS>,
    MI_Filament<FILAMENT_PC>,
    MI_Filament<FILAMENT_FLEX>,
    MI_Filament<FILAMENT_HIPS>,
    MI_Filament<FILAMENT_PP>,
    MI_Filament<FILAMENT_NONE>>;

template <class T>
FILAMENT_t make_preheat_dialog(string_view_utf8 caption) {
    set_last_preheated_filament(FILAMENT_NONE);
    T dlg(caption, Screens::Access()->Get(), GuiDefaults::RectScreenBody);
    create_blocking_dialog_from_normal_window(dlg);
    return get_last_preheated_filament();
}

FILAMENT_t gui_dlg_preheat(string_view_utf8 caption) {
    return make_preheat_dialog<Screen>(caption);
}

FILAMENT_t gui_dlg_preheat_autoselect_if_able(string_view_utf8 caption) {
    const FILAMENT_t fil = get_filament();
    if (fil == FILAMENT_NONE) {
        //no filament selected
        return gui_dlg_preheat(caption);
    } else {
        //when filament is known, but heating is off, just turn it on and do not ask
        marlin_vars_t *p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
        if (p_vars->target_nozzle != filaments[fil].nozzle) {
            marlin_gcode_printf("M104 S%d", (int)filaments[fil].nozzle);
            marlin_gcode_printf("M140 S%d", (int)filaments[fil].heatbed);
        }
    }
    return fil;
}

//no return option
FILAMENT_t gui_dlg_preheat_forced(string_view_utf8 caption) {
    return make_preheat_dialog<ScreenNoRet>(caption);
}

//no return option
FILAMENT_t gui_dlg_preheat_autoselect_if_able_forced(string_view_utf8 caption) {
    const FILAMENT_t fil = get_filament();
    if (fil == FILAMENT_NONE) {
        //no filament selected
        return gui_dlg_preheat_forced(caption);
    } else {
        //when filament is known, but heating is off, just turn it on and do not ask
        marlin_vars_t *p_vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TTEM_NOZ));
        if (p_vars->target_nozzle != filaments[fil].nozzle) {
            marlin_gcode_printf("M104 S%d", (int)filaments[fil].nozzle);
            marlin_gcode_printf("M140 S%d", (int)filaments[fil].heatbed);
        }
    }
    return fil;
}
