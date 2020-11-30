/**
 * @file mock_windows.hpp
 * @author Radek Vana
 * @brief Mock windows and screens to check window registration and capture
 * @date 2020-11-30
 */

#pragma once
#include "screen.hpp"
#include "window_dlg_popup.hpp"
#include "IDialog.hpp"
#include "window_dlg_strong_warning.hpp"

struct MockMsgBox : public AddSuperWindow<IDialog> {
    MockMsgBox(Rect16 rc)
        : AddSuperWindow<IDialog>(rc) {}
};

class MockStrongDialog : public AddSuperWindow<window_dlg_strong_warning_t> {
public:
    void Show(string_view_utf8 txt) { show(txt); }

    static MockStrongDialog &ShowHotendFan() {
        static MockStrongDialog dlg;
        dlg.Show(_(HotendFanErrorMsg));
        return dlg;
    }

    static MockStrongDialog &ShowPrintFan() {
        static MockStrongDialog dlg;
        dlg.Show(_(PrintFanErrorMsg));
        return dlg;
    }

    static MockStrongDialog &ShowHeaterTimeout() {
        static MockStrongDialog dlg;
        dlg.Show(_(HeaterTimeoutMsg));
        return dlg;
    }

    static MockStrongDialog &ShowUSBFlashDisk() {
        static MockStrongDialog dlg;
        dlg.Show(_(USBFlashDiskError));
        return dlg;
    }
};

struct MockScreen : public AddSuperWindow<screen_t> {
    window_t w_first; // just so w0 is not first
    window_t w0;
    window_t w1;
    window_t w2;
    window_t w3;
    window_t w_last; // just so w3 is not last

    MockScreen()
        : w_first(this, GuiDefaults::RectHeader) // header is not hidden behind dialog
        , w0(this, Rect16(20, 20, 10, 10) + GuiDefaults::RectScreenBody.Top())
        , w1(this, Rect16(20, 40, 10, 10) + GuiDefaults::RectScreenBody.Top())
        , w2(this, Rect16(40, 20, 10, 10) + GuiDefaults::RectScreenBody.Top())
        , w3(this, Rect16(40, 40, 10, 10) + GuiDefaults::RectScreenBody.Top())
        , w_last(this, GuiDefaults::RectHeader) {} // header is not hidden behind dialog

    void ParrentCheck();
    void LinkedListCheck(size_t popup_cnt = 0, size_t dialog_cnt = 0, size_t strong_dialog_cnt = 0);
    void BasicCheck(size_t popup_cnt = 0, size_t dialog_cnt = 0, size_t strong_dialog_cnt = 0);
};
