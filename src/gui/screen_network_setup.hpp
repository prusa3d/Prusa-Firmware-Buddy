#pragma once

#include "screen_fsm.hpp"
#include <footer_line.hpp>

class ScreenNetworkSetup final : public ScreenFSM {

public:
    using Phase = PhaseNetworkSetup;

public:
    ScreenNetworkSetup();
    ~ScreenNetworkSetup();

protected:
    inline PhaseNetworkSetup get_phase() const {
        return GetEnumFromPhaseIndex<PhaseNetworkSetup>(fsm_base_data.GetPhase());
    }

protected:
    void create_frame() override;
    void destroy_frame() override;
    void update_frame() override;

    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;
};
