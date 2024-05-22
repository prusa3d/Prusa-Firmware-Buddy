#pragma once

#include "status_footer.hpp"

#include <common/static_storage.hpp>
#include <common/fsm_base_types.hpp>
#include <dialogs/radio_button_fsm.hpp>
#include <screen.hpp>
#include <window_header.hpp>

class ScreenColdPull final : public AddSuperWindow<screen_t> {
public:
    using FrameStorage = StaticStorage<436>;

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
