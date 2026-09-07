#pragma once

#include <footer_line.hpp>
#include <client_fsm_types.hpp>
#include <window_frame.hpp>
#include <window_text.hpp>
#include <radio_button_fsm.hpp>
#include <optional>
#include <error_codes.hpp>

/**
 * Standard layout frame.
 * Contains:
 * - Orange left-aligned title (alignment can be changed)
 * - Gray line (visible if title is not empty)
 * - Left-aligned text (alignment can be changed)
 * - A FSM radio
 */
class FramePrompt {

public:
    FramePrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info);
    FramePrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, const Align_t info_alignment, const Align_t title_alignment);

    /** Takes title and text from the error code
     *  Construct the frame with \param error_code_mapper function (phase -> ErrCode), to extract useful information from ErrDesc related to given phase.
     */
    FramePrompt(window_frame_t *parent, FSMAndPhase fsm_phase, std::optional<ErrCode> (*error_code_mapper)(FSMAndPhase fsm_phase), const Align_t info_alignment = Align_t::LeftTop(), const Align_t title_alignment = Align_t::LeftBottom());

    /**
     * Used by WithFooter<>
     * @param footer to add to vertical stack
     */
    void add_footer(FooterLine &footer); // Used by WithFooter<>

protected:
    window_text_t title;
    BasicWindow title_line;
    window_text_t info;
    RadioButtonFSM radio;
};
