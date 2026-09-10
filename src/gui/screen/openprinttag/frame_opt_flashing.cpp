/// @file
#include "frame_opt_flashing.hpp"

#include <gui/auto_layout.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <guiconfig/guiconfig.h>

static_assert(HAS_LARGE_DISPLAY(), "This frame has been only done for a large display");

namespace buddy::openprinttag {

namespace {
    constexpr std::array layout {
        StackLayoutItem { .height = 32, .margin_side = 16, .margin_top = 24, .margin_bottom = 8 },
        standard_stack_layout::for_progress_bar,
        StackLayoutItem { .height = 32, .margin_side = 16 },
        StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16, .margin_top = 8 },
        standard_stack_layout::for_radio,
    };

    constexpr int progress_bar_corner_radius = 4;

    constexpr const char *label_updating = N_("The OpenPrintTag reader is being updated. It will be available once the update finishes.");
    constexpr const char *label_stalled = N_("The update is not progressing. Check the reader connection.");
} // namespace

FrameOPTFlashing::FrameOPTFlashing(window_frame_t *parent)
    : window_frame_t { parent, GuiDefaults::RectScreenNoHeader }
    , title_(this, Rect16 {}, is_multiline::no, is_closed_on_click_t::no, _("Updating reader firmware"))
    , progress_bar_(this, Rect16 {}, COLOR_BRAND, COLOR_DARK_GRAY, progress_bar_corner_radius)
    , progress_number_(this, Rect16 {}, 0, "%.0f%%", Font::big)
    , label_(this, Rect16 {}, is_multiline::yes, is_closed_on_click_t::no, _(label_updating))
    , radio_(this, Rect16 {}, PhaseResponses { Response::Ok }) {

    title_.set_font(GuiDefaults::FontBig);
    title_.SetAlignment(Align_t::Center());
    progress_number_.SetAlignment(Align_t::Center());
    label_.SetAlignment(Align_t::CenterTop());

    parent->CaptureNormalWindow(*this);
    CaptureNormalWindow(radio_);

    auto windows = std::to_array<window_t *>({ &title_, &progress_bar_, &progress_number_, &label_, &radio_ });
    layout_vertical_stack(GetRect(), windows, layout);
}

void FrameOPTFlashing::set_progress(uint8_t percent) {
    progress_bar_.set_progress_percent(percent);
    progress_number_.SetValue(percent);
}

void FrameOPTFlashing::set_stalled(bool stalled) {
    label_.SetText(_(stalled ? label_stalled : label_updating));
}

void FrameOPTFlashing::set_done() {
    title_.SetText(_("Reader updated"));
    set_progress(100);
    label_.SetText({});
}

} // namespace buddy::openprinttag
