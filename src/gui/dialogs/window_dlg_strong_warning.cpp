//window_dlg_strong_warning.cpp

#include "window_dlg_strong_warning.hpp"
#include "display_helper.h"
#include "resource.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "dialog_response.hpp"

const PhaseResponses dlg_responses = { Response::Continue, Response::_none, Response::_none, Response::_none };

window_dlg_strong_warning_t::window_dlg_strong_warning_t()
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreen, IDialog::IsStrong::yes)
    , header(this, _(Title))
    , footer(this)
    , icon(this, IDR_PNG_exposure_times_48px, { 120 - 24, 48 })
    , text(this, { 0, 104, 240, 120 }, is_multiline::yes)
    , button(this, get_radio_button_rect(rect) - Rect16::Top_t(64), &dlg_responses, &ph_txt_continue) {
}

void window_dlg_strong_warning_t::setIcon(int16_t resId) {
    icon.SetIdRes(resId);
}

void window_dlg_strong_warning_t::show(string_view_utf8 txt) {
    if (!GetParent()) {
        window_t *parent = Screens::Access()->Get();
        if (parent) {
            parent->RegisterSubWin(*this);
            text.SetText(txt);
        }
    }
}

void window_dlg_strong_warning_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (!GetParent())
        return;
    if (event == GUI_event_t::CLICK) { //todo use timer
        GetParent()->UnregisterSubWin(*this);
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

void window_dlg_strong_warning_t::ShowHotendFan() {
    static window_dlg_strong_warning_t dlg;
    Sound_Play(eSOUND_TYPE::StandardAlert);
    dlg.show(_(HotendFanErrorMsg));
    dlg.setIcon(IDR_PNG_fan_error);
}

void window_dlg_strong_warning_t::ShowPrintFan() {
    static window_dlg_strong_warning_t dlg;
    Sound_Play(eSOUND_TYPE::StandardAlert);
    dlg.show(_(PrintFanErrorMsg));
    dlg.setIcon(IDR_PNG_fan_error);
}

void window_dlg_strong_warning_t::ShowHeatersTimeout() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(_(HeaterTimeoutMsg));
    dlg.setIcon(IDR_PNG_exposure_times_48px);
}

void window_dlg_strong_warning_t::ShowUSBFlashDisk() {
    static window_dlg_strong_warning_t dlg;
    Sound_Play(eSOUND_TYPE::StandardAlert);
    dlg.show(_(USBFlashDiskError));
    dlg.setIcon(IDR_PNG_usb_error);
}
