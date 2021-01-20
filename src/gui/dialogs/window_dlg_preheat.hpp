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

filament_t gui_dlg_preheat(string_view_utf8 caption);
filament_t gui_dlg_preheat_autoselect_if_able(string_view_utf8 caption);
filament_t gui_dlg_preheat_forced(string_view_utf8 caption);                    //no return option
filament_t gui_dlg_preheat_autoselect_if_able_forced(string_view_utf8 caption); //no return option

class DialogMenuPreheat : public AddSuperWindow<IDialogMarlin> {
    class I_MI_Filament : public WI_LABEL_t {
    public:
        I_MI_Filament(string_view_utf8 long_name)
            : WI_LABEL_t(long_name, 0, is_enabled_t::yes, is_hidden_t::no) {}

    protected:
        void click_at(filament_t filament_index);
    };

    template <filament_t T>
    class MI_Filament : public I_MI_Filament {
    public:
        MI_Filament()
            : I_MI_Filament(_(filaments[size_t(T)].long_name)) {}

    protected:
        virtual void click(IWindowMenu & /*window_menu*/) override {
            click_at(T);
        }
    };

    //TODO try to use HIDDEN on return and filament_t::NONE
    using MenuContainer = WinMenuContainer<MI_RETURN,
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
    using MenuContainerNoRet = WinMenuContainer<
        MI_Filament<filament_t::PLA>,
        MI_Filament<filament_t::PETG>,
        MI_Filament<filament_t::ASA>,
        MI_Filament<filament_t::ABS>,
        MI_Filament<filament_t::PC>,
        MI_Filament<filament_t::FLEX>,
        MI_Filament<filament_t::HIPS>,
        MI_Filament<filament_t::PP>>;

    MenuContainer container;
    MenuContainerNoRet containerNoRet;
    window_menu_t menu;
    window_header_t header; //can be hidden

public:
    DialogMenuPreheat(string_view_utf8 name);

protected:
    virtual bool change(uint8_t phs, uint8_t progress_tot, uint8_t progress) override;
};
