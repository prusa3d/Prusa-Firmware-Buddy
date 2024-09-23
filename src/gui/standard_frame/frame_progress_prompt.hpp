#pragma once

#include <client_fsm_types.h>

#include <window_progress.hpp>
#include <window_frame.hpp>
#include <window_text.hpp>
#include <radio_button_fsm.hpp>

/**
 * Standard layout frame.
 * Contains:
 * - Centered title
 * - Progress bar
 * - Centered text (alignment can be changed)
 * - A FSM radio
 */
class FrameProgressPrompt : public window_frame_t {

public:
    FrameProgressPrompt(window_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, Align_t info_alignment = Align_t::CenterTop());

protected:
    window_text_t title;
    window_numberless_progress_t progress_bar;
    window_text_t info;
    RadioButtonFSM radio;
};
