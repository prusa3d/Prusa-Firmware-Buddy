#include "frame_prompt.hpp"

#include <gui/auto_layout.hpp>
#include <find_error.hpp>
#include <bsod/bsod.h>
#include <guiconfig/GuiDefaults.hpp>

namespace {

static constexpr std::array layout_no_footer {
    StackLayoutItem { .height = 32, .margin_side = 16, .margin_bottom = 4 },
    StackLayoutItem { .height = 2, .margin_side = 16 },
    StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16, .margin_top = 16 },
    standard_stack_layout::for_radio,
};

static constexpr std::array layout_only_footer {
    standard_stack_layout::for_footer,
};

static constexpr std::array layout_with_footer = stdext::array_concat(layout_no_footer, layout_only_footer);

static_assert(layout_no_footer.size() + 1 == layout_with_footer.size(), "Layout without footer should be exactly one item (the footer) smaller than layout with footer");

} // namespace

FramePrompt::FramePrompt(window_frame_t *parent, DisplayConfig display_config)
    : title(parent, {}, is_multiline::yes, is_closed_on_click_t::no, {})
    , title_line(parent, {})
    , info(parent, {}, is_multiline::yes, is_closed_on_click_t::no, {})
    , radio(parent, {}) //
{
    title.SetAlignment(display_config.title_alignment);
    title.set_font(GuiDefaults::FontBig);
    title.SetTextColor(COLOR_BRAND);

    info.SetAlignment(display_config.info_alignment);
#if HAS_MINI_DISPLAY()
    info.set_font(Font::small);
#endif
    title_line.SetBackColor(COLOR_DARK_GRAY);

    parent->CaptureNormalWindow(radio);

    std::array<window_t *, layout_no_footer.size()> windows_no_footer { &title, &title_line, &info, &radio };
    layout_vertical_stack(parent->GetRect(), windows_no_footer, layout_no_footer);
}

FramePrompt::FramePrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &txt_title, const string_view_utf8 &txt_info, DisplayConfig display_config)
    : FramePrompt { parent, display_config } {
    radio.set_fsm_and_phase(fsm_phase);
    set_texts(txt_title, txt_info);
}

FramePrompt::FramePrompt(window_frame_t *parent, FSMAndPhase fsm_phase, std::optional<ErrCode> (*error_code_mapper)(FSMAndPhase fsm_phase), DisplayConfig display_config)
    : FramePrompt { parent, display_config } {
    radio.set_fsm_and_phase(fsm_phase);

    // Extracting information: Phase -> corresponding error code -> message
    const auto err = find_error(error_code_mapper(fsm_phase).value());
    set_texts(_(err.err_title), _(err.err_text));
}

void FramePrompt::set_texts(const string_view_utf8 &txt_title, const string_view_utf8 &txt_info) {
    title.SetText(txt_title);
    info.SetText(txt_info);
    title_line.set_visible(txt_title.getFirstUtf8Char() != '\0');
}

void FramePrompt::add_footer(FooterLine &footer) {
    std::array<window_t *, layout_with_footer.size()> windows_with_footer { &title, &title_line, &info, &radio, &footer };
    layout_vertical_stack(title.GetParent()->GetRect(), windows_with_footer, layout_with_footer);
}
