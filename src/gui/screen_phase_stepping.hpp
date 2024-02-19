#pragma once

#include <radio_button_fsm.hpp>
#include <screen.hpp>
#include <window_text.hpp>
#include <window_icon.hpp>
#include <window_qr.hpp>
#include <array>

class ScreenPhaseStepping : public AddSuperWindow<screen_t> {
    window_text_t text;
    window_text_t link;
    window_icon_t icon_phone;
    window_qr_t qr;
    RadioButtonFsm<PhasesPhaseStepping> radio;
    std::array<char, 150> text_buffer;
    fsm::BaseData fsm_base_data;

    void flip_layout();
    void do_change(fsm::BaseData);

public:
    ScreenPhaseStepping();
    virtual ~ScreenPhaseStepping();

    static ScreenPhaseStepping *GetInstance();
    void Change(fsm::BaseData);

    void InitState(screen_init_variant) final;
    screen_init_variant GetCurrentState() const final;
};
