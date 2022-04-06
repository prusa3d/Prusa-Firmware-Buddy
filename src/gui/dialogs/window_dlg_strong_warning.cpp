//window_dlg_strong_warning.cpp

#include "window_dlg_strong_warning.hpp"
#include "display_helper.h"
#include "resource.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "client_response_texts.hpp"

const PhaseResponses dlg_responses = { Response::Continue, Response::_none, Response::_none, Response::_none };

static constexpr Rect16 textRectIcon = { 0, 104, 240, 120 };

window_dlg_strong_warning_t::window_dlg_strong_warning_t()
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreen, IDialog::IsStrong::yes)
    , header(this, _(Title))
    , footer(this)
    , icon(this, IDR_PNG_exposure_times_48px, { 120 - 24, 48 })
    , text(this, textRectIcon, is_multiline::yes)
    , button(this, GuiDefaults::GetButtonRect(GetRect()) - Rect16::Top_t(64), dlg_responses, &ph_txt_continue) {
}

void window_dlg_strong_warning_t::setIcon(int16_t resId) {
    icon.SetIdRes(resId);
    if (IDR_NULL == resId)
        text.SetRect(GuiDefaults::RectScreenBody);
    else
        text.SetRect(textRectIcon);
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

void window_dlg_strong_warning_t::ShowHotendTempDiscrepancy() {
    static window_dlg_strong_warning_t dlg;
    Sound_Play(eSOUND_TYPE::StandardAlert);
    dlg.header.SetText(_(TitleNozzle));
    dlg.show(_(HotendTempDiscrepancyMsg));
    dlg.setIcon(IDR_NULL);
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
