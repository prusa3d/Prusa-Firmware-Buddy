/**
 * @file screen_change_filter.cpp
 * @brief Display or link to how to change enclosure filter on XL
 */

#include "screen_change_filter.hpp"
#include "ScreenHandler.hpp"
#include <config_store/store_instance.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include "img_resources.hpp"
#if XL_ENCLOSURE_SUPPORT()
    #include "xl_enclosure.hpp"
#endif

// This is visible only on XL (ILI9488 layout) so we don't need vertical display layout option
namespace {
constexpr PhaseResponses responses_change_filter = { Response::Back, Response::Done, Response::_none, Response::_none };
constexpr const uint16_t text_padding = 40;
constexpr const uint16_t visual_delimeter = 10;

constexpr const Rect16 descr_rect = Rect16(
    text_padding,
    GuiDefaults::HeaderHeight + visual_delimeter,
    GuiDefaults::ScreenWidth - 2 * text_padding - GuiDefaults::QRSize - visual_delimeter,
    GuiDefaults::MsgBoxLayoutRect.Height() / 2);
constexpr const Rect16 help_rect = Rect16(descr_rect.Left(), descr_rect.Bottom() + 20, descr_rect.Width(), descr_rect.Height());
constexpr const Rect16 qr_rect = Rect16(descr_rect.Right() + visual_delimeter, descr_rect.Top(), GuiDefaults::QRSize, GuiDefaults::QRSize);

constexpr const char *txt_descr = N_("If the filter on your enclosure is close to its expiration time (600 h), please change it for a new one.");
constexpr const char *qr_link = "prusa.io/xl-filter";
constexpr const char *txt_help = N_("To learn how to change XL enclosure filter, please visit:\nprusa.io/xl-filter");
}; // namespace

ScreenChangeFilter::ScreenChangeFilter()
    : screen_t()
    , header(this)
    , description(this, descr_rect, is_multiline::yes)
    , help(this, help_rect, is_multiline::yes)
    , qr(this, qr_rect, Align_t::RightTop(), qr_link)
    , radio(this, GuiDefaults::GetButtonRect(GetRect()), responses_change_filter) {
    CaptureNormalWindow(radio);

    header.SetIcon(&img::info_16x16);
    header.SetText(_("FILTER CHANGE"));

    description.SetAlignment(Align_t::LeftTop());
    description.SetText(_(txt_descr));

    help.SetAlignment(Align_t::LeftTop());
    help.SetText(_(txt_help));
}

void ScreenChangeFilter::windowEvent([[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CHILD_CLICK:
        switch (event_conversion_union { .pvoid = param }.response) {
        case Response::Done:
#if XL_ENCLOSURE_SUPPORT()
            // Reset EEPROM variable holding HEPA filter expiration counter (600h)
            xl_enclosure.resetFilterTimer();
#endif
            break;
        default:
            break;
        }
        Screens::Access()->Close();
        break;
    default:
        break;
    }
}
