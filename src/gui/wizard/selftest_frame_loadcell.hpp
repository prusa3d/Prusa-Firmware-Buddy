/**
 * @file selftest_frame_loadcell.hpp
 * @author Radek Vana
 * @brief part of screen containing loadcell selftest
 * @date 2021-12-03
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "radio_button.hpp"
#include "status_footer.hpp"

/**
 * @brief
 * lot of texts are moving originally wanted to use invalidation rectangles
 * but it is just simpler to make many texts witch cost rectangles
 * but it will consume lot of RAM
 */
class SelftestFrameLoadcell : public AddSuperWindow<SelftestFrameNamedWithRadio> {
    FooterLine footer;
    window_wizard_progress_t progress;

    window_icon_t icon_hand;
    window_text_t text_phase;

    window_text_t text_prebig;
    WindowBlinkingText text_big; // BIG, possibly blinking number

    window_text_t text_result; // in middle of screen

protected:
    virtual void change() override;

public:
    SelftestFrameLoadcell(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
