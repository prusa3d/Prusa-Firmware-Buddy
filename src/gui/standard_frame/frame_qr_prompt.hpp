#pragma once

#include <client_fsm_types.h>

#include <qr.hpp>
#include <window_icon.hpp>
#include <window_frame.hpp>
#include <window_text.hpp>
#include <radio_button_fsm.hpp>

/**
 * Standard layout frame with a QR code.
 * Contains:
 * - Left aligned text
 * - QR code & link
 * - A FSM radio
 */
class FrameQRPrompt : public window_frame_t {

public:
    FrameQRPrompt(window_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &info_text, const char *qr_suffix);

protected:
    window_text_t info;
    window_text_t link;
    window_icon_t icon_phone;
    QRDynamicStringWindow<32> qr;
    RadioButtonFSM radio;

    std::array<char, 32> link_buffer;
};
