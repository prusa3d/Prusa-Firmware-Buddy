/*
 * window_dlg_preheat.hpp
 *
 *  Created on: 2019-11-18
 *      Author: Vana Radek
 */

#pragma once

#include "filament.hpp"
#include "../../lang/string_view_utf8.hpp"
#include "screen_menu.hpp"
#include "IDialogMarlin.hpp"
#include "fsm_preheat_type.hpp"
#include "MItem_tools.hpp"

namespace NsPreheat {

inline constexpr size_t info_len = sizeof("999/999 "); // extra space at the end is intended
class I_MI_Filament : public WiInfo<info_len> {
public:
    I_MI_Filament(string_view_utf8 name, unsigned t_noz, unsigned t_bed);

protected:
    void click_at(filament::Type filament_index);
};

template <filament::Type T>
class MI_Filament : public I_MI_Filament {
public:
    MI_Filament()
        : I_MI_Filament(_(filament::get_name(T)), filament::get_description(T).nozzle, filament::get_description(T).heatbed) {}

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        click_at(T);
    }
};

class MI_RETURN : public IWindowMenuItem {
    static constexpr const char *const label = N_("Return");

public:
    MI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};

class MI_COOLDOWN : public IWindowMenuItem {
public:
    MI_COOLDOWN();

protected:
    virtual void click(IWindowMenu &window_menu);
};

using WinMenuContainer = WinMenuContainer<
    MI_RETURN,
    MI_Filament<filament::Type::PLA>,
    MI_Filament<filament::Type::PETG>,
    MI_Filament<filament::Type::ASA>,
    MI_Filament<filament::Type::PC>,
    MI_Filament<filament::Type::PVB>,
    MI_Filament<filament::Type::ABS>,
    MI_Filament<filament::Type::HIPS>,
    MI_Filament<filament::Type::PP>,
    MI_Filament<filament::Type::PA>,
    MI_Filament<filament::Type::FLEX>,
    MI_COOLDOWN>;

}; // namespace NsPreheat

class DialogMenuPreheat : public AddSuperWindow<IDialogMarlin> {
    NsPreheat::WinMenuContainer menu_container;
    window_menu_t menu;
    window_header_t header;

public:
    DialogMenuPreheat(fsm::BaseData data);

protected:
    static PreheatData get_type(fsm::BaseData data);
    static string_view_utf8 get_title(fsm::BaseData data);
};
