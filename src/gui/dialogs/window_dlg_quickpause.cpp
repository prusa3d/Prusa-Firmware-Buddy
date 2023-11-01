/**
 * @file window_dlg_quickpause.cpp
 */

#include "window_dlg_quickpause.hpp"
#include "../../lang/string_view_utf8.hpp"
#include "client_response.hpp"
#include "img_resources.hpp"
#include "marlin_vars.hpp"
#include "marlin_server_shared.h"

constexpr static const char quick_pause_txt[] = N_("Waiting for the user. Press \"Resume\" once the printer is ready.");

DialogQuickPause::DialogQuickPause(fsm::BaseData)
    : AddSuperWindow<IDialogMarlin>(GuiDefaults::RectScreenBody)
    , icon(this, GuiDefaults::MessageIconRect, &img::warning_48x48)
    , text(this, GuiDefaults::MessageTextRect, is_multiline::yes, is_closed_on_click_t::yes, _(quick_pause_txt))
    , gcode_name(this, Rect16(GuiDefaults::MsgBoxLayoutRect.Left(), 45, GuiDefaults::MsgBoxLayoutRect.Width(), 21))
    , radio(this, GuiDefaults::GetButtonRect_AvoidFooter(GuiDefaults::RectScreenBody), PhasesQuickPause::QuickPaused) {

    if (marlin_vars()->print_state == marlin_server::State::Printing) {
        auto lock = MarlinVarsLockGuard();
        static char buff[FILE_NAME_BUFFER_LEN] = { 0 };
        marlin_vars()->media_LFN.copy_to(buff, FILE_NAME_BUFFER_LEN, lock);
        gcode_name.SetText(_(buff));
    }
    CaptureNormalWindow(radio);
}
