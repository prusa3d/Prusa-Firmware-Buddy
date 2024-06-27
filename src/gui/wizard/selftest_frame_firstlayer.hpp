/**
 * @file selftest_frame_firstlayer.hpp
 * @brief part of screen containing first layer calibration
 */

#pragma once

#include "selftest_frame.hpp"
#include "status_footer.hpp"
#include "liveadjust_z.hpp"
#include <window_progress.hpp>

class SelftestFrameFirstLayer : public SelftestFrame {
    static constexpr const char *text_str = N_(
        "Once the printer starts extruding plastic, adjust the nozzle height by turning the knob until the filament sticks to the print sheet.");
    StatusFooter footer;
    window_text_t text;
    window_numberless_progress_t progress;
    WindowLiveAdjustZ_withText live_z;

protected:
    virtual void change() override;

public:
    SelftestFrameFirstLayer(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
