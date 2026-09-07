#pragma once

#include "footer_line.hpp"
#include <client_fsm_types.hpp>

#include <window_progress.hpp>
#include <window_frame.hpp>
#include <window_text.hpp>
#include <radio_button_fsm.hpp>
#include <optional>
#include <error_codes.hpp>

/**
 * Standard layout frame.
 * Contains:
 * - Centered title
 * - Progress bar
 * - Centered text (alignment can be changed)
 * - A FSM radio
 */
class FrameProgressPrompt {

public:
    FrameProgressPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, Align_t info_alignment = Align_t::CenterTop());

    /** Takes title and text from the error code
     *  Construct the frame with \param error_code_mapper function (phase -> ErrCode), to extract useful information from ErrDesc related to given phase.
     */
    FrameProgressPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, std::optional<ErrCode> (*error_code_mapper)(const FSMAndPhase fsm_phase));

    /**
     * Used by WithFooter<>
     * @param footer to add to vertical stack
     */
    void add_footer(FooterLine &footer);

protected:
    window_text_t title;
    WindowProgressBar progress_bar;
    window_text_t info;
    RadioButtonFSM radio;
};
