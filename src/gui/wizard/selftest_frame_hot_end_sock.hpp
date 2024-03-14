/**
 * @file selftest_frame_hot_end_sock.hpp
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"

class SelftestFrameHotEndSock : public AddSuperWindow<SelftestFrameWithRadio> {
    window_text_t text;
    window_text_t text_nozzle;
    window_text_t text_nozzle_value;
    window_text_t text_sock;
    window_text_t text_sock_value;

protected:
    virtual void change() override;

public:
    SelftestFrameHotEndSock(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
