//window_dlg_strong_warning.cpp

#include "window_dlg_strong_warning.hpp"
#include "display_helper.h"
#include "i18n.h"
#include "ScreenHandler.hpp"

window_dlg_strong_warning_t::window_dlg_strong_warning_t()
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreenBodyNoFoot, IDialog::IsStrong::yes)
    , text(this, GuiDefaults::RectScreenBodyNoFoot, is_multiline::yes, is_closed_on_click_t::no) {
}

void window_dlg_strong_warning_t::show(string_view_utf8 txt) {
    if (!GetParent()) {
        window_t *parent = Screens::Access()->Get();
        if (parent) {
            parent->RegisterSubWin(this);
            text.SetText(txt);
        }
    }
}

void window_dlg_strong_warning_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (!GetParent())
        return;
    if (event == GUI_event_t::CLICK) { //todo use timer
        GetParent()->UnregisterSubWin(this);
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

void window_dlg_strong_warning_t::ShowHotendFan() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(_(HotendFanErrorMsg));
}

void window_dlg_strong_warning_t::ShowPrintFan() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(_(PrintFanErrorMsg));
}

void window_dlg_strong_warning_t::ShowHeaterTimeout() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(_(HeaterTimeoutMsg));
}

void window_dlg_strong_warning_t::ShowUSBFlashDisk() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(_(USBFlashDiskError));
}
