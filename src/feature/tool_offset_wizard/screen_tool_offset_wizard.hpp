#pragma once

#include <screen_fsm.hpp>

class ScreenToolOffsetWizard final : public ScreenFSM {
public:
    ScreenToolOffsetWizard();
    ~ScreenToolOffsetWizard();

    inline PhaseToolOffsetsCalibration get_phase() const {
        return GetEnumFromPhaseIndex<PhaseToolOffsetsCalibration>(fsm_base_data.GetPhase());
    }

protected:
    void create_frame() final;
    void destroy_frame() final;
    void update_frame() final;
};
