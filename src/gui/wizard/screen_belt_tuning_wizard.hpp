#pragma once

#include <array>

#include <screen_fsm.hpp>

class ScreenBeltTuningWizard final : public ScreenFSM {

public:
    static constexpr int graph_width = GuiDefaults::ScreenWidth / 2;
    static constexpr int graph_height = 96;

public:
    ScreenBeltTuningWizard();
    ~ScreenBeltTuningWizard();

protected:
    inline PhaseBeltTuning get_phase() const {
        return GetEnumFromPhaseIndex<PhaseBeltTuning>(fsm_base_data.GetPhase());
    }

protected:
    void create_frame() override;
    void destroy_frame() override;
    void update_frame() override;

public:
    /// Height of the column (in pixels) for each X coordinate of the graph
    std::array<uint8_t, graph_width> graph_data;
};
