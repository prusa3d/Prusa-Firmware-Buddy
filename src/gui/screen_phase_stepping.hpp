#pragma once

#include <common/static_storage.hpp>
#include <radio_button_fsm.hpp>
#include <screen.hpp>
#include <window_frame.hpp>
#include <window_header.hpp>

class ScreenPhaseStepping : public AddSuperWindow<screen_t> {
public:
    using FrameStorage = StaticStorage<420>;

private:
    FrameStorage frame_storage;
    window_header_t header;
    window_frame_t inner_frame;
    RadioButtonFsm<PhasesPhaseStepping> radio;
    fsm::BaseData fsm_base_data;

    void create_frame();
    void destroy_frame();
    void update_frame();
    void do_change(fsm::BaseData);

public:
    ScreenPhaseStepping();
    virtual ~ScreenPhaseStepping();

    static ScreenPhaseStepping *GetInstance();
    void Change(fsm::BaseData);

    void InitState(screen_init_variant) final;
    screen_init_variant GetCurrentState() const final;
};
