/// @file
#pragma once

#include <gui/frame_calibration_common.hpp>
#include <client_response.hpp>
#include <radio_button_fsm.hpp>
#include <window_frame.hpp>

class FrameCalibrationTextWithImage {

protected:
    FrameCalibrationTextWithImage(window_frame_t *parent, FSMAndPhase fsm_phase, string_view_utf8 txt, Rect16::Top_t top, const img::Resource *icon_res, uint16_t icon_width)
        : text(parent, Rect16(WizardDefaults::col_0, top, WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginLeft - WizardDefaults::MarginRight - icon_width, WizardDefaults::row_h * 8), is_multiline::yes, is_closed_on_click_t::no, txt)
        , icon(parent, icon_res, point_i16_t(WizardDefaults::RectSelftestFrame.Width() - WizardDefaults::MarginRight - icon_width, top))
        , radio(parent, WizardDefaults::RectRadioButton(0), fsm_phase) {
        parent->CaptureNormalWindow(radio);
    }

private:
    window_text_t text;
    window_icon_t icon;
    RadioButtonFSM radio;
};
