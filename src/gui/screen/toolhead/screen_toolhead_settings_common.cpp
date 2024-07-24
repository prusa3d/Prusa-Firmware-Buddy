#include "screen_toolhead_settings_common.hpp"

using namespace screen_toolhead_settings;

bool screen_toolhead_settings::msgbox_confirm_change(Toolhead toolhead) {
    // Single toolhead change -> no need to confirm
    if (toolhead != all_toolheads) {
        return true;
    }

    return MsgBoxQuestion(_("This change will apply to all toolheads. Do you want to continue?"), Responses_YesNo) == Response::Yes;
}
