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
    I_MI_Filament(FilamentType filament_type);

protected:
    virtual void click(IWindowMenu &) final;

protected:
    const FilamentTypeParameters filament_params;
    const FilamentType filament_type;
};

template <PresetFilamentType T>
class MI_Filament : public I_MI_Filament {
public:
    MI_Filament()
        : I_MI_Filament(T) {}
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
    MI_Filament<PresetFilamentType::PLA>,
    MI_Filament<PresetFilamentType::PETG>,
    MI_Filament<PresetFilamentType::ASA>,
    MI_Filament<PresetFilamentType::PC>,
    MI_Filament<PresetFilamentType::PVB>,
    MI_Filament<PresetFilamentType::ABS>,
    MI_Filament<PresetFilamentType::HIPS>,
    MI_Filament<PresetFilamentType::PP>,
    MI_Filament<PresetFilamentType::PA>,
    MI_Filament<PresetFilamentType::FLEX>,
    MI_COOLDOWN>;

}; // namespace NsPreheat

class DialogMenuPreheat : public IDialogMarlin {
    NsPreheat::WinMenuContainer menu_container;
    window_menu_t menu;
    window_header_t header;

public:
    DialogMenuPreheat(fsm::BaseData data);

protected:
    static PreheatData get_type(fsm::BaseData data);
    static string_view_utf8 get_title(fsm::BaseData data);
};
