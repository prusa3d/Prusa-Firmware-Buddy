/// @file
#include <gui/screen/initial/screen_remove_heatbed_screws.hpp>

#include <option/has_heatbed_screws_during_transport.h>
static_assert(HAS_HEATBED_SCREWS_DURING_TRANSPORT());

#include <ScreenHandler.hpp>
#include <config_store/store_instance.hpp>
#include <img_resources.hpp>
#include <lang/i18n.h>
#include <window_msgbox.hpp>

bool ScreenRemoveHeatbedScrews::should_show() {
    return !config_store().heatbed_screws_removal_approved.get();
}

ScreenRemoveHeatbedScrews::ScreenRemoveHeatbedScrews()
    : PseudoScreenCallback {
        [] {
            static constexpr point_ui16_t icon_point = point_ui16_t(40, 20);
            MsgBoxIconned msgbox(
                Rect16(0, 0, GuiDefaults::ScreenWidth, GuiDefaults::ScreenHeight),
                icon_point,
                Responses_Ok,
                0,
                _("Before using the 3D printer, it is necessary to remove all screws, that secure the heated bed during transport.\n\nThe screws are marked with a sticker."),
                is_multiline::yes,
                &img::ac_heatbed_screw_80x246,
                is_closed_on_click_t::yes);
            Screens::Access()->gui_loop_until_dialog_closed();
            config_store().heatbed_screws_removal_approved.set(true);
        },
    } {}
