#include "frame_qr_prompt.hpp"

#include <img_resources.hpp>
#include <guiconfig/wizard_config.hpp>
#include <find_error.hpp>
#include <auto_layout.hpp>

namespace {
static constexpr uint8_t qr_size = GuiDefaults::QRSize;
static constexpr uint8_t txt_height = WizardDefaults::txt_h;
static constexpr uint8_t spacing = 16;
// Inset of the inner frame (no bottom margin - radio sits right below). Reuses HeaderPadding
// so the content lines up with the header inset and adapts per display.
static constexpr int16_t inner_frame_margin_side = GuiDefaults::HeaderPadding.left;
static constexpr int16_t inner_frame_margin_top = GuiDefaults::HeaderPadding.top;
static constexpr auto txt_details = N_("More details at");
static constexpr auto txt_scan_me = N_("Scan me!");

static constexpr std::array layout_no_footer {
    StackLayoutItem {
        .height = StackLayoutItem::stretch,
        .margin_side = inner_frame_margin_side,
        .margin_top = inner_frame_margin_top,
    }, // inner_frame (text + QR + details + link)
    standard_stack_layout::for_radio,
};
static constexpr std::array layout_only_footer {
    standard_stack_layout::for_footer,
};
static constexpr std::array layout_with_footer = stdext::array_concat(layout_no_footer, layout_only_footer);
static_assert(layout_no_footer.size() + 1 == layout_with_footer.size(), "Layout without footer should be exactly one item (the footer) smaller than layout with footer");

} // namespace

FrameQRPrompt::FrameQRPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &info_text)
    : inner_frame(parent, parent->GetRect())
    , info(&inner_frame, {}, is_multiline::yes, is_closed_on_click_t::no, info_text)
    , scan_me(&inner_frame, {}, is_multiline::no, is_closed_on_click_t::no, txt_scan_me)
    , qr(&inner_frame, {})
    , details(&inner_frame, {}, is_multiline::no, is_closed_on_click_t::no, txt_details)
    , link(&inner_frame, {})
    , radio(parent, {}, fsm_phase) //
{
    qr.SetAlignment(Align_t::CenterTop());
    parent->CaptureNormalWindow(radio);
#if HAS_LARGE_DISPLAY()
    scan_me.SetAlignment(Align_t::CenterTop());
#else // MINI
    scan_me.SetAlignment(Align_t::LeftCenter());
#endif
    details.SetAlignment(Align_t::LeftBottom());
    link.SetAlignment(Align_t::LeftTop());
    link.set_font(Font::small);
    details.set_font(Font::small);

    std::array<window_t *, layout_no_footer.size()> windows_no_footer { &inner_frame, &radio };
    layout_vertical_stack(parent->GetRect(), windows_no_footer, layout_no_footer);

    layout_contents();
}

FrameQRPrompt::FrameQRPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, const string_view_utf8 &info_text, const char *qr_suffix)
    : FrameQRPrompt { parent, fsm_phase, info_text } {
    debug_assert(qr_suffix);

    StringBuilder(link_buffer)
        .append_string("prusa.io/")
        .append_string(qr_suffix)
        .check();
    link.SetText(string_view_utf8::MakeRAM(link_buffer.data()));

    qr.get_string_builder()
        .append_string("https://prusa.io/")
        .append_string(qr_suffix)
        .check();
}

void FrameQRPrompt::layout_contents() {
    const Rect16 f = inner_frame.GetRect();
    const int16_t row = txt_height; // height of one details/link row

#if HAS_LARGE_DISPLAY()
    // Two columns: text on the left, QR on the right.
    const int16_t left_w = f.Width() - qr_size - spacing;
    const int16_t right_x = f.Left() + left_w + spacing;

    // Right column: QR on top, "Scan me!" right below it.
    qr.SetRect(Rect16(right_x, f.Top(), qr_size, qr_size));
    scan_me.SetRect(Rect16(right_x, f.Top() + qr_size, qr_size, row + spacing));

    // Left column: info on top, "More details at" + link pinned to the bottom.
    info.SetRect(Rect16(f.Left(), f.Top(), left_w, f.Height() - 2 * row));
    details.SetRect(Rect16(f.Left(), f.Top() + f.Height() - 2 * row, left_w, row));
    link.SetRect(Rect16(f.Left(), f.Top() + f.Height() - row, left_w, row));
#else // MINI
    // Single column (too narrow for two): info on top, then "More details at" +
    // link, then QR with "Scan me!" beside it pinned to the bottom.
    const int16_t qr_top = f.Top() + f.Height() - qr_size;
    qr.SetRect(Rect16(f.Left(), qr_top, qr_size, qr_size));
    scan_me.SetRect(Rect16(f.Left() + qr_size + spacing, qr_top, f.Width() - qr_size - spacing, qr_size));

    details.SetRect(Rect16(f.Left(), qr_top - spacing - 2 * row, f.Width(), row));
    link.SetRect(Rect16(f.Left(), qr_top - spacing - row, f.Width(), row));
    info.SetRect(Rect16(f.Left(), f.Top(), f.Width(), (qr_top - spacing - 2 * row) - f.Top()));
#endif
}

FrameQRPrompt::FrameQRPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, ErrCode err_code)
    : FrameQRPrompt(parent, fsm_phase, string_view_utf8::MakeNULLSTR()) {

    const auto err = find_error(err_code);

    info.SetText(_(err.err_text));

    // link's internal buffer is used instead of link_buffer
    link.set_error_code(err_code);

    qr.set_error_code(err.err_code);
}

// Phase -> corresponding error code -> message. Phases without an error code must not
// use this constructor: .value() BSODs (throws bad_optional_access) if there is none.
FrameQRPrompt::FrameQRPrompt(window_frame_t *parent, FSMAndPhase fsm_phase, std::optional<ErrCode> (*error_code_mapper)(FSMAndPhase fsm_phase))
    : FrameQRPrompt(parent, fsm_phase, error_code_mapper(fsm_phase).value()) {}

void FrameQRPrompt::add_footer(FooterLine &footer) {
    std::array<window_t *, layout_with_footer.size()> windows_with_footer { &inner_frame, &radio, &footer };
    layout_vertical_stack(radio.GetParent()->GetRect(), windows_with_footer, layout_with_footer);
    layout_contents();
}
