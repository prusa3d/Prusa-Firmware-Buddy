#include "frame_prompt.hpp"

#include <gui/auto_layout.hpp>

FramePrompt::FramePrompt(window_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, Align_t info_alignment)
    : window_frame_t(parent, parent->GetRect())
    , title(this, {}, is_multiline::yes, is_closed_on_click_t::no, txt_title)
    , info(this, {}, is_multiline::yes, is_closed_on_click_t::no, txt_info)
    , radio(this, {}, fsm_phase) //
{
    title.SetAlignment(Align_t::CenterBottom());
    title.set_font(GuiDefaults::FontBig);

    info.SetAlignment(info_alignment);
#if HAS_MINI_DISPLAY()
    info.set_font(Font::small);
#endif

    CaptureNormalWindow(radio);
    static_cast<window_frame_t *>(parent)->CaptureNormalWindow(*this);

    static constexpr std::initializer_list layout {
        StackLayoutItem { .height = 64 },
        StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16, .margin_top = 16 },
        standard_stack_layout::for_radio,
    };
    layout_vertical_stack(GetRect(), { &title, &info, &radio }, layout);
}
