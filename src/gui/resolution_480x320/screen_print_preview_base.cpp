/**
 * @file screen_print_preview_base.cpp
 */

#include "screen_print_preview_base.hpp"
#include "img_resources.hpp"
#include "gcode_description.hpp" // TITLE_HEIGHT

static constexpr Rect16 title_rect {
    GuiDefaults::PreviewThumbnailRect.Left(),
    GuiDefaults::HeaderHeight + 8,
    display::GetW() - 2 * GuiDefaults::PreviewThumbnailRect.Left(),
    TITLE_HEIGHT
};

static constexpr Rect16 vertical_radio_buttons_rect {
    GuiDefaults::PreviewThumbnailRect.Right() + (display::GetW() - GuiDefaults::PreviewThumbnailRect.Right() - RadioButtonPreview::vertical_buttons_width) / 2,
    GuiDefaults::PreviewThumbnailRect.Top(),
    RadioButtonPreview::vertical_buttons_width,
    display::GetH() - GuiDefaults::PreviewThumbnailRect.Top()
};

ScreenPrintPreviewBase::ScreenPrintPreviewBase()
    : header(this, _("PRINT"))
    , title_text(this, title_rect)
    , radio(this, vertical_radio_buttons_rect) {

    header.SetIcon(&img::print_16x16);
}
