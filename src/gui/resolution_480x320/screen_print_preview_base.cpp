/**
 * @file screen_print_preview_base.cpp
 */

#include "screen_print_preview_base.hpp"
#include "png_resources.hpp"
#include "gcode_description.hpp" // TITLE_HEIGHT

static constexpr Rect16 title_rect {
    GuiDefaults::PreviewThumbnailRect.Left(),
    GuiDefaults::HeaderHeight + 8,
    display::GetW() - 2 * GuiDefaults::PreviewThumbnailRect.Left(),
    TITLE_HEIGHT
};

static constexpr Rect16 vertical_radio_buttons_rect {
    GuiDefaults::PreviewThumbnailRect.Right() + (display::GetW() - GuiDefaults::PreviewThumbnailRect.Right() - GuiDefaults::ButtonIconSize) / 2,
    GuiDefaults::PreviewThumbnailRect.Top(),
    GuiDefaults::ButtonIconSize,
    display::GetH() - GuiDefaults::PreviewThumbnailRect.Top()
};

ScreenPrintPreviewBase::ScreenPrintPreviewBase()
    : header(this, _("PRINT"))
    , title_text(this, title_rect)
    , radio(this, vertical_radio_buttons_rect) {

    header.SetIcon(&png::print_16x16);
}
