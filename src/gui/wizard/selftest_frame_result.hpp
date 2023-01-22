/**
 * @file selftest_frame_result.hpp
 * @author Radek Vana
 * @brief result part of the selftest
 * @date 2022-01-24
 */
#pragma once

#include "selftest_frame.hpp"
#include "selftest_view.hpp"
#include "selftest_view_item_separator.hpp"
#include "selftest_view_item_text.hpp"
#include "selftest_result_fans.hpp"
#include "selftest_result_axis.hpp"
#include "selftest_result_heaters.hpp"
#include "selftest_result_eth.hpp"
#include "selftest_result_wifi.hpp"
#include "scroll_bar.hpp"
#include "window_text.hpp"

class SelftestFrameResult : public AddSuperWindow<SelftestFrame> {
    window_text_t msg;
    SelfTestView view;
    ScrollBar bar; // TODO it does not show, partially unimplemented???

    //used int to be able to do std::min/max
    Rect16::Height_t height_draw_offset;
    Rect16::Height_t virtual_view_height;

    ResultFans fans;
    ResultAxis axis;
    ResultHeaters heaters;
    ResultEth eth;
    ResultWifi wifi;

    static constexpr int pixels_per_knob_move = 32;

public:
    SelftestFrameResult(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
