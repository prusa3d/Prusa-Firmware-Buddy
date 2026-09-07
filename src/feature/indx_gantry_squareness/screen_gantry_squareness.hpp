/// @file
#pragma once

#include <marlin_server_types/fsm/gantry_squareness_phases.hpp>
#include <screen_fsm.hpp>

class ScreenGantrySquareness final : public ScreenFSM {
public:
    ScreenGantrySquareness();
    ~ScreenGantrySquareness();

    inline PhaseGantrySquareness get_phase() const {
        return GetEnumFromPhaseIndex<PhaseGantrySquareness>(fsm_base_data.GetPhase());
    }

protected:
    void create_frame() final;
    void destroy_frame() final;
    void update_frame() final;
};
