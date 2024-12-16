/// @file
#include <gui/frame_calibration_common.hpp>

FrameInstructions::FrameInstructions(window_t *parent, const string_view_utf8 &txt)
    : text(parent, rect_frame, is_multiline::yes, is_closed_on_click_t::no, txt) {
}

void FrameInstructions::update(fsm::PhaseData) {}

#if HAS_ATTACHABLE_ACCELEROMETER()

FrameConnectToBoard::FrameConnectToBoard(window_t *parent)
    : FrameInstructions {
        parent,
        _(
            "Accelerometer is not responding. "
            "Turn off the printer and make sure the accelerometer cable is connected to the main board. "
            "You can also abort the calibration and continue using the printer with previous settings."),
    } {}

FrameWaitForExtruderTemperature::FrameWaitForExtruderTemperature(window_t *parent)
    : text_above { parent, rect_frame_top, is_multiline::no, is_closed_on_click_t::no, _("Waiting for nozzle to cool down") }
    , text_below { parent, rect_frame_bottom } {
    text_above.SetAlignment(Align_t::CenterTop());
    text_below.SetAlignment(Align_t::CenterTop());
    text_below.SetBlinkColor(COLOR_AZURE);
}

void FrameWaitForExtruderTemperature::update(fsm::PhaseData data) {
    const auto current_temperature = ((data[0] << 8) | data[1]) % 1000;

    snprintf(text_below_buffer.data(), text_below_buffer.size(), "%3d °C", current_temperature);
    text_below.SetText(string_view_utf8::MakeRAM(text_below_buffer.data()));
    text_below.Invalidate();
}

FrameAttachToExtruder::FrameAttachToExtruder(window_t *parent)
    : FrameInstructions {
        parent,
        _(
            "Firmly attach the accelerometer to the extruder "
            "(remove silicone sock if necessary). "
            "In the next step, extruder will start moving "
            "and calibration data will be collected."),
    } {}

FrameAttachToBed::FrameAttachToBed(window_t *parent)
    : FrameInstructions {
        parent,
        _(
            "Firmly attach the accelerometer to the heatbed. "
            "In the next step, heatbed will start moving "
            "and calibration data will be collected."),
    } {}

#endif
