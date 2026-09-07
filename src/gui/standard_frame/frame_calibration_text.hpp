/// @file
#pragma once

#include <gui/frame_calibration_common.hpp>
#include <client_response.hpp>
#include <radio_button_fsm.hpp>
#include <window_frame.hpp>

class FrameCalibrationText {

protected:
    FrameCalibrationText(window_frame_t *parent, FSMAndPhase fsm_phase, string_view_utf8 txt, const Rect16::Top_t top)
        : text(parent, Rect16(WizardDefaults::col_0, top, WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginRight - WizardDefaults::MarginLeft, WizardDefaults::row_h * 8), is_multiline::yes, is_closed_on_click_t::no, txt)
        , radio(parent, WizardDefaults::RectRadioButton(0), fsm_phase) {
        parent->CaptureNormalWindow(radio);
    }

private:
    window_text_t text;
    RadioButtonFSM radio;
};
