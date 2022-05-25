//window_dlg_strong_warning.cpp

#include "window_dlg_strong_warning.hpp"
#include "display_helper.h"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "client_response_texts.hpp"

std::bitset<window_dlg_strong_warning_t::types::count_> window_dlg_strong_warning_t::shown;
window_dlg_strong_warning_t::types window_dlg_strong_warning_t::on_top = window_dlg_strong_warning_t::types::count_;

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

void window_dlg_strong_warning_t::show(types type) {
    if (shown[type])
        return;
    shown[type] = true;
    on_top = type;

    icon.SetIdRes(icon_title_text[type].icon);
    header.SetText(_(icon_title_text[type].title));
    if (IDR_NULL == icon_title_text[type].icon)
        text.SetRect(GuiDefaults::RectScreenBody);
    else
        text.SetRect(textRectIcon);
    text.SetText(_(icon_title_text[type].text));

    if (!GetParent()) {
        window_t *parent = Screens::Access()->Get();
        if (parent) {
            Sound_Play(eSOUND_TYPE::StandardAlert);
            parent->RegisterSubWin(*this);
        }
    }
}

void window_dlg_strong_warning_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (!GetParent())
        return;
    if (event == GUI_event_t::CLICK) { //todo use timer
        shown[on_top] = false;         // remove from mask
        on_top = types::count_;        //erase on_top

        if (shown.any()) {
            for (types t = types(0); t < types::count_; t = types(t + 1)) {
                if (shown[t]) {
                    show(t);
                    break;
                }
            }
        } else {
            GetParent()->UnregisterSubWin(*this);
        }

    } else {
        SuperWindowEvent(sender, event, param);
    }
}

void window_dlg_strong_warning_t::ShowHotendFan() {
    Instance().show(HotendFan);
}

void window_dlg_strong_warning_t::ShowPrintFan() {
    Instance().show(PrintFan);
}

void window_dlg_strong_warning_t::ShowHotendTempDiscrepancy() {
    Instance().show(HotendTempDiscrepancy);
}

void window_dlg_strong_warning_t::ShowHeatersTimeout() {
    Instance().show(HeatersTimeout);
}

void window_dlg_strong_warning_t::ShowUSBFlashDisk() {
    Instance().show(USBFlashDisk);
}
