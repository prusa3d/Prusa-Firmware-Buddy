#pragma once

#include <client_fsm_types.hpp>

#include <qr.hpp>
#include <window_icon.hpp>
#include <window_frame.hpp>
#include <window_text.hpp>
#include <radio_button_fsm.hpp>
#include <text_error_url.hpp>
#include <error_codes.hpp>
#include <optional>
#include <footer_line.hpp>

/**
 * Standard layout frame with a QR code.
 * The inner frame is split into two columns:
 * - Left: info text + "More details at" + link
 * - Right: QR code + "Scan me!"
 * A FSM radio sits below the inner frame.
 */
class FrameQRPrompt {

public:
    FrameQRPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &info_text, const char *qr_suffix);

    /** Takes info text and QR suffix from the error code.
     *  Construct the frame with \param error_code_mapper function (phase -> ErrCode), to extract useful information from ErrDesc related to given phase.
     */
    FrameQRPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, std::optional<ErrCode> (*error_code_mapper)(FSMAndPhase fsm_phase));

    /** Takes info text and QR suffix directly from \param err_code.
     *  Useful when the error code does not follow from the phase alone (e.g. warnings, where it comes from the FSM data).
     */
    FrameQRPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, ErrCode err_code);

    /**
     * Used by WithFooter<>
     * @param footer to add to vertical stack
     */
    void add_footer(FooterLine &footer);

    /// Override the left-column info text. Useful when the message has to be
    /// built at runtime (e.g. a formatted count) instead of the error's default text.
    void set_info_text(const string_view_utf8 &text) { info.SetText(text); }

protected:
    /// Positions all sub-windows inside the (already laid-out) inner_frame.
    void layout_contents();

    window_frame_t inner_frame; // holds the text on the left and the QR code with ScanMe on the right
    window_text_t info;
    window_text_t scan_me;
    QRErrorUrlWindow qr;
    window_text_t details; // "More details at"
    TextErrorUrlWindow link; // The link at the bottom of the screen
    RadioButtonFSM radio;

    std::array<char, 48> link_buffer;
};
