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
#include "MItem_tools.hpp"

namespace NsPreheat {
class I_MI_Filament : public WI_INFO_t {
public:
    I_MI_Filament(string_view_utf8 name, unsigned t_noz, unsigned t_bed);

protected:
    void click_at(filament_t filament_index);
};

template <filament_t T>
class MI_Filament : public I_MI_Filament {
public:
    MI_Filament()
        : I_MI_Filament(_(Filaments::Get(T).name), Filaments::Get(T).nozzle, Filaments::Get(T).heatbed) {}

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

#define ALL_FILAMENTS MI_Filament<filament_t::PLA>,  \
                      MI_Filament<filament_t::PETG>, \
                      MI_Filament<filament_t::ASA>,  \
                      MI_Filament<filament_t::PC>,   \
                      MI_Filament<filament_t::PVB>,  \
                      MI_Filament<filament_t::ABS>,  \
                      MI_Filament<filament_t::HIPS>, \
                      MI_Filament<filament_t::PP>,   \
                      MI_Filament<filament_t::FLEX>

//TODO try to use HIDDEN on return and filament_t::NONE
//has both return and cooldown
using MenuContainerHasRetCool = WinMenuContainer<MI_RETURN, ALL_FILAMENTS, MI_COOLDOWN>;

//has return
using MenuContainerHasRet = WinMenuContainer<MI_RETURN, ALL_FILAMENTS>;

//has cooldown
using MenuContainerHasCool = WinMenuContainer<ALL_FILAMENTS, MI_COOLDOWN>;

// no extra fields
using MenuContainer = WinMenuContainer<ALL_FILAMENTS>;
};

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
    DialogMenuPreheat(string_view_utf8 name, PreheatData type);

protected:
    virtual bool change(uint8_t phase, fsm::PhaseData data) override;
};
