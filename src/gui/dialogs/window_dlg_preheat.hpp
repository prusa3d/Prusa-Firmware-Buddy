/*
 * window_dlg_preheat.hpp
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#pragma once

#include "filament.h"
#include "../../lang/string_view_utf8.hpp"
#include "screen_menu.hpp"
#include "DialogStateful.hpp"
#include "MItem_tools.hpp"

FILAMENT_t gui_dlg_preheat(string_view_utf8 caption);
FILAMENT_t gui_dlg_preheat_autoselect_if_able(string_view_utf8 caption);
FILAMENT_t gui_dlg_preheat_forced(string_view_utf8 caption);                    //no return option
FILAMENT_t gui_dlg_preheat_autoselect_if_able_forced(string_view_utf8 caption); //no return option

//TODO try to use HIDDEN on return and FILAMENT_NONE
using MenuContainer = WinMenuContainer<MI_RETURN,
    MI_Filament<FILAMENT_PLA>,
    MI_Filament<FILAMENT_PETG>,
    MI_Filament<FILAMENT_ASA>,
    MI_Filament<FILAMENT_ABS>,
    MI_Filament<FILAMENT_PC>,
    MI_Filament<FILAMENT_FLEX>,
    MI_Filament<FILAMENT_HIPS>,
    MI_Filament<FILAMENT_PP>,
    MI_Filament<FILAMENT_NONE>>;

// is used in firstlay calibration and print preview, does not have return and cooldown
using MenuContainerNoRet = WinMenuContainer<
    MI_Filament<FILAMENT_PLA>,
    MI_Filament<FILAMENT_PETG>,
    MI_Filament<FILAMENT_ASA>,
    MI_Filament<FILAMENT_ABS>,
    MI_Filament<FILAMENT_PC>,
    MI_Filament<FILAMENT_FLEX>,
    MI_Filament<FILAMENT_HIPS>,
    MI_Filament<FILAMENT_PP>>;

class DialogMenuPreheat : public AddSuperWindow<IDialogMarlin> {

    MenuContainer container;
    MenuContainerNoRet containerNoRet;
    window_menu_t menu;
    window_header_t header; //can be hidden

public:
    DialogMenuPreheat(string_view_utf8 name);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    virtual bool change(uint8_t phs, uint8_t progress_tot, uint8_t progress) override;

    virtual void draw() {
        super::draw();
    }
};
