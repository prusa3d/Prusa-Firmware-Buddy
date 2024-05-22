/**
 * @file selftest_frame_hotend_specify.hpp
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"

class SelftestFrameHotendSpecify : public AddSuperWindow<SelftestFrameWithRadio> {
    window_text_t text;
    window_text_t text_nozzle;
    window_text_t text_nozzle_value;
    window_text_t text_hotend;
    window_text_t text_hotend_value;

protected:
    virtual void change() override;

public:
    SelftestFrameHotendSpecify(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
