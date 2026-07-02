/// @file
#include "screen_opt_detected.hpp"
#include "opt_gui_manager.hpp"

#include <img_resources.hpp>
#include <ScreenHandler.hpp>

#include <feature/openprinttag/data_utils.hpp>
#include <screen/openprinttag/screen_opt_info.hpp>
#include <screen/openprinttag/opt_request_wizard.hpp>

static_assert(HAS_LARGE_DISPLAY(), "This screen has been only done for a large display");

namespace buddy::openprinttag {

ScreenOPTDetected::ScreenOPTDetected(ToolTag tag)
    : ScreenOPTPrompt { N_("OPENPRINTTAG DETECTED") }
    , frame_ { this, _("OpenPrintTag detected") }
    , tag_(tag) {

    frame_.setup_tag(tag, [] {
        // If tag reading fails, just close the screen
        // This is also called on tag lost - in which case we close the screen and any dialog on top of it
        Screens::Access()->close_screen_and_dialog();
    });

    frame_.setup_radio({ Response::Ok, Response::Load, Response::Details }, [this](Response r) {
        switch (r) {

        case Response::Ok:
            close_screen();
            break;

        case Response::Load: {
            if (FilamentType::for_tool(tag_.tool()) != FilamentType::none) {
                StringViewUtf8Parameters<4> params;
                if (MsgBoxWarning(_("Filament is already loaded to tool %i. Unload first?").formatted(params, tag_.tool().display_index()), { Response::Unload, Response::Cancel }) != Response::Unload) {
                    return;
                }

                marlin_client::gcode_printf("M702 T%i", tag_.tool().to_raw());
            }

            marlin_client::gcode_printf("M701 W2 T%i G%i", tag_.tool().to_raw(), tag_.uid_hash());
            close_screen();
            break;
        }

        case Response::Details:
            Screens::Access()->Open(screen_opt_info_ephemeral_creator(tag_.tool()));
            break;

        default:
            bsod_unreachable();
        }
    });
}

} // namespace buddy::openprinttag
