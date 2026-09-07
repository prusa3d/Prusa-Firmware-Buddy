/**
 * @file window_dlg_quickpause.cpp
 */

#include "window_dlg_quickpause.hpp"
#include "../../lang/string_view_utf8.hpp"
#include "client_response.hpp"
#include "img_resources.hpp"
#include "marlin_vars.hpp"
#include "marlin_server_shared.h"
#include <buddy/filename_defs.hpp>
#include <find_error.hpp>

constexpr static const char *quick_pause_txt = find_error(ErrCode::CONNECT_QUICK_PAUSE).err_text;

DialogQuickPause::DialogQuickPause(fsm::BaseData data)
    : IDialogMarlin(GuiDefaults::RectScreenBody)
    , icon(this, GuiDefaults::MessageIconRect, &img::warning_48x48)
    , text(this, GuiDefaults::MessageTextRect, is_multiline::yes, is_closed_on_click_t::yes, _(quick_pause_txt))
    , gcode_name(this, Rect16(GuiDefaults::MsgBoxLayoutRect.Left(), 45, GuiDefaults::MsgBoxLayoutRect.Width(), 21))
    , radio(this, GuiDefaults::GetButtonRect_AvoidFooter(GuiDefaults::RectScreenBody), PhasesQuickPause::QuickPaused) {

    const char *msg = nullptr;
    memcpy(&msg, (uint32_t *)data.GetData().data(), sizeof(uint32_t));
    if (msg) {
        // Custom message (e.g. the "Empty Wastebin" prompt). Show it instead of the default
        // quick-pause text and hide the gcode-name field - that one belongs to the M0 quick-pause
        // presentation and would render confusingly over a custom dialog.
        // Run it through gettext: a translatable (N_-marked) string gets localized; raw strings
        // not in the catalog pass through unchanged.
        text.SetText(_(msg));
        gcode_name.Hide();
    } else if (marlin_vars().print_state == marlin_server::State::Printing) {
        auto lock = MarlinVarsLockGuard();
        static char buff[filename_defs::filename_buffer_size] = { 0 };
        marlin_vars().media_LFN.copy_to(buff, filename_defs::filename_buffer_size, lock);
        gcode_name.SetText(string_view_utf8::MakeRAM(buff));
    }

    CaptureNormalWindow(radio);
}
