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
#include "DialogStateful.hpp"
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
        : I_MI_Filament(_(filament::get_description(T).name), filament::get_description(T).nozzle, filament::get_description(T).heatbed) {}

protected:
    virtual void click(IWindowMenu & /*window_menu*/) override {
        click_at(T);
    }
};

class MI_RETURN : public WI_LABEL_t {
    static constexpr const char *const label = N_("Return");

public:
    MI_RETURN();

protected:
    virtual void click(IWindowMenu &window_menu);
};

class MI_COOLDOWN : public WI_LABEL_t {
public:
    MI_COOLDOWN();

protected:
    virtual void click(IWindowMenu &window_menu);
};

#if PRINTER_IS_PRUSA_iX
    #define ALL_FILAMENTS MI_Filament<filament::Type::PLA>,     \
                          MI_Filament<filament::Type::PETG>,    \
                          MI_Filament<filament::Type::PETG_NH>, \
                          MI_Filament<filament::Type::ASA>,     \
                          MI_Filament<filament::Type::PC>,      \
                          MI_Filament<filament::Type::PVB>,     \
                          MI_Filament<filament::Type::ABS>,     \
                          MI_Filament<filament::Type::HIPS>,    \
                          MI_Filament<filament::Type::PP>,      \
                          MI_Filament<filament::Type::PA>,      \
                          MI_Filament<filament::Type::FLEX>
#else
    #define ALL_FILAMENTS MI_Filament<filament::Type::PLA>,  \
                          MI_Filament<filament::Type::PETG>, \
                          MI_Filament<filament::Type::ASA>,  \
                          MI_Filament<filament::Type::PC>,   \
                          MI_Filament<filament::Type::PVB>,  \
                          MI_Filament<filament::Type::ABS>,  \
                          MI_Filament<filament::Type::HIPS>, \
                          MI_Filament<filament::Type::PP>,   \
                          MI_Filament<filament::Type::PA>,   \
                          MI_Filament<filament::Type::FLEX>
#endif

// TODO try to use HIDDEN on return and filament_t::NONE
// has both return and cooldown
using MenuContainerHasRetCool = WinMenuContainer<MI_RETURN, ALL_FILAMENTS, MI_COOLDOWN>;

// has return
using MenuContainerHasRet = WinMenuContainer<MI_RETURN, ALL_FILAMENTS>;

// has cooldown
using MenuContainerHasCool = WinMenuContainer<ALL_FILAMENTS, MI_COOLDOWN>;

// no extra fields
using MenuContainer = WinMenuContainer<ALL_FILAMENTS>;
}; // namespace NsPreheat

class DialogMenuPreheat : public AddSuperWindow<IDialogMarlin> {
    // single memory space for all containers to save RAM
    // it is not static to save RAM (it uses mem space for dialogs)
    // allocated by placement new
    // !!! must be before menu, so initializer list in ctor can use it !!!
    std::aligned_union<0, NsPreheat::MenuContainerHasRetCool, NsPreheat::MenuContainerHasRet, NsPreheat::MenuContainerHasCool, NsPreheat::MenuContainer>::type container_mem_space;

    IWinMenuContainer *newContainer(PreheatData type);

    window_menu_t menu;
    window_header_t header;

public:
    DialogMenuPreheat(fsm::BaseData data);

protected:
    static PreheatData get_type(fsm::BaseData data);
    static string_view_utf8 get_title(fsm::BaseData data);
};
