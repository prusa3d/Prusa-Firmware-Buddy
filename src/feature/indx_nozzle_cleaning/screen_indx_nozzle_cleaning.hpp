#pragma once

#include <screen_fsm.hpp>

class ScreenIndxNozzleCleaning final : public ScreenFSM {
public:
    ScreenIndxNozzleCleaning();
    ~ScreenIndxNozzleCleaning();

    inline PhaseNozzleCleaning get_phase() const {
        return GetEnumFromPhaseIndex<PhaseNozzleCleaning>(fsm_base_data.GetPhase());
    }

protected:
    void create_frame() final;
    void destroy_frame() final;
    void update_frame() final;
};
