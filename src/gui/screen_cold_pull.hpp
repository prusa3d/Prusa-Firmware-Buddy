#pragma once

#include <common/static_storage.hpp>
#include <common/fsm_base_types.hpp>
#include <dialogs/radio_button_fsm.hpp>
#include <footer_line.hpp>
#include <screen.hpp>
#include <window_header.hpp>

class ScreenColdPull final : public AddSuperWindow<screen_t> {
public:
#if HAS_TOOLCHANGER()
    using FrameStorage = StaticStorage<1060>;
#else
    using FrameStorage = StaticStorage<436>;
#endif

private:
    FrameStorage frame_storage;

    window_header_t header;
    FooterLine footer;
    RadioButtonFsm<PhasesColdPull> radio;

    window_frame_t inner_frame;

    fsm::BaseData fsm_base_data;

    void create_frame();
    void destroy_frame();
    void update_frame();
    void do_change(fsm::BaseData);

public:
    ScreenColdPull();
    virtual ~ScreenColdPull();

    static ScreenColdPull *GetInstance();
    void Change(fsm::BaseData);

    void InitState(screen_init_variant) final;
    screen_init_variant GetCurrentState() const final;
};
