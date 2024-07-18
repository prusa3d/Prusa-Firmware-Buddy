/**
 * @file window_dlg_quickpause.cpp
 */

#include "window_dlg_quickpause.hpp"
#include "../../lang/string_view_utf8.hpp"
#include "client_response.hpp"
#include "img_resources.hpp"
#include "marlin_vars.hpp"
#include "marlin_server_shared.h"
#include <find_error.hpp>

constexpr static const char *quick_pause_txt = find_error(ErrCode::CONNECT_QUICK_PAUSE).err_text;

DialogQuickPause::DialogQuickPause(fsm::BaseData data)
    : IDialogMarlin(GuiDefaults::RectScreenBody)
    , icon(this, GuiDefaults::MessageIconRect, &img::warning_48x48)
    , text(this, GuiDefaults::MessageTextRect, is_multiline::yes, is_closed_on_click_t::yes, _(quick_pause_txt))
    , gcode_name(this, Rect16(GuiDefaults::MsgBoxLayoutRect.Left(), 45, GuiDefaults::MsgBoxLayoutRect.Width(), 21))
    , radio(this, GuiDefaults::GetButtonRect_AvoidFooter(GuiDefaults::RectScreenBody), PhasesQuickPause::QuickPaused) {

    if (marlin_vars().print_state == marlin_server::State::Printing) {
        auto lock = MarlinVarsLockGuard();
        static char buff[FILE_NAME_BUFFER_LEN] = { 0 };
        marlin_vars().media_LFN.copy_to(buff, FILE_NAME_BUFFER_LEN, lock);
        gcode_name.SetText(string_view_utf8::MakeRAM(buff));
    }

    const char *msg;
    memcpy(&msg, (uint32_t *)data.GetData().data(), sizeof(uint32_t));
    if (msg) {
        text.SetText(string_view_utf8::MakeRAM((const uint8_t *)msg));
    }

    CaptureNormalWindow(radio);
}
