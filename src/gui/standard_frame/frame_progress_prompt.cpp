#include "frame_progress_prompt.hpp"
#include <find_error.hpp>
#include <gui/auto_layout.hpp>
#include <bsod/bsod.h>

namespace {
static constexpr std::array layout_no_footer {
    StackLayoutItem { .height = 64 },
    standard_stack_layout::for_progress_bar,
    StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16, .margin_top = 16 },
    standard_stack_layout::for_radio,
};
static constexpr std::array layout_only_footer {
    standard_stack_layout::for_footer,
};
static constexpr std::array layout_with_footer = stdext::array_concat(layout_no_footer, layout_only_footer);
static_assert(layout_no_footer.size() + 1 == layout_with_footer.size(), "Layout without footer should be exactly one item (the footer) smaller than layout with footer");
} // namespace

FrameProgressPrompt::FrameProgressPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, Align_t info_alignment)
    : title(parent, {}, is_multiline::yes, is_closed_on_click_t::no, txt_title)
    , progress_bar(parent, {}, COLOR_BRAND, COLOR_GRAY)
    , info(parent, {}, is_multiline::yes, is_closed_on_click_t::no, txt_info)
    , radio(parent, {}, fsm_phase) {

    title.SetAlignment(Align_t::CenterBottom());
    title.set_font(GuiDefaults::FontBig);

    info.SetAlignment(info_alignment);
#if HAS_MINI_DISPLAY()
    info.set_font(Font::small);
#endif

    static_cast<window_frame_t *>(parent)->CaptureNormalWindow(radio);

    std::array<window_t *, layout_no_footer.size()> windows_no_footer { &title, &progress_bar, &info, &radio };
    layout_vertical_stack(parent->GetRect(), windows_no_footer, layout_no_footer);
}

FrameProgressPrompt::FrameProgressPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, std::optional<ErrCode> (*error_code_mapper)(const FSMAndPhase fsm_phase))
    : FrameProgressPrompt(parent, fsm_phase, string_view_utf8::MakeNULLSTR(), string_view_utf8::MakeNULLSTR()) {
    // Extracting information: Phase -> corresponding error code -> message
    const auto err_code = error_code_mapper(fsm_phase);
    if (!err_code.has_value()) {
        bsod_unreachable(); // Some phases do not have corresponding error codes - they should not be called with this constructor
    }

    const auto err = find_error(err_code.value());

    info.SetText(_(err.err_text));
    title.SetText(_(err.err_title));
}

void FrameProgressPrompt::add_footer(FooterLine &footer) {
    std::array<window_t *, layout_with_footer.size()> windows_with_footer { &title, &progress_bar, &info, &radio, &footer };
    layout_vertical_stack(title.GetParent()->GetRect(), windows_with_footer, layout_with_footer);
}
