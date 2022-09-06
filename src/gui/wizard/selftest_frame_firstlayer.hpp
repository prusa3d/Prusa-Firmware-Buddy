/**
 * @file selftest_frame_firstlayer.hpp
 * @brief part of screen containing first layer calibration
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"
#include "window_print_progress.hpp"
#include "liveadjust_z.hpp"

class SelftestFrameFirstLayer : public AddSuperWindow<SelftestFrame> {
    static constexpr const char *text_str = N_(
        "Once the printer starts extruding plastic, adjust the nozzle height by turning the knob until the filament sticks to the print sheet.");
    StatusFooter footer;
    window_text_t text;
    WindowPrintProgress progress;
    WindowLiveAdjustZ_withText live_z;

protected:
    virtual void change() override;

public:
    SelftestFrameFirstLayer(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
