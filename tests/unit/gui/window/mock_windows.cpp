/**
 * @file mock_windows.cpp
 * @author Radek Vana
 * @date 2020-11-30
 */

#include "mock_windows.hpp"
#include "ScreenHandler.hpp"
void MockScreen::ParrentCheck() const {
    //check parrent
    REQUIRE(w_first.GetParent() == this);
    REQUIRE(w_last.GetParent() == this);
    REQUIRE(w0.GetParent() == this);
    REQUIRE(w1.GetParent() == this);
    REQUIRE(w2.GetParent() == this);
    REQUIRE(w3.GetParent() == this);
}

void MockScreen::LinkedListCheck(size_t popup_cnt, size_t dialog_cnt, size_t strong_dialog_cnt) const {
    //check linked list
    REQUIRE(getFirstNormal() == &(w_first));
    REQUIRE(getLastNormal() == &(w_last));
    REQUIRE(getFirstNormal()->GetNext() == &(w0));
    REQUIRE(w0.GetNext() == &(w1));
    REQUIRE(w1.GetNext() == &(w2));
    REQUIRE(w2.GetNext() == &(w3));
    REQUIRE(w3.GetNext() == &(w_last));

    window_t *pLast = getLastNormal();

    checkPtrRange(pLast, dialog_cnt, GetFirstDialog(), GetLastDialog());
    checkPtrRange(pLast, strong_dialog_cnt, GetFirstStrongDialog(), GetLastStrongDialog());
    checkPtrRange(pLast, popup_cnt, GetFirstPopUp(), GetLastPopUp());

    REQUIRE(pLast->GetNext() == nullptr);
}

void MockScreen::BasicCheck(size_t popup_cnt, size_t dialog_cnt, size_t strong_dialog_cnt) const {
    //check parrent
    ParrentCheck();

    //check IsHiddenBehindDialog()
    REQUIRE_FALSE(w_first.IsHiddenBehindDialog());
    REQUIRE_FALSE(w_last.IsHiddenBehindDialog());
    REQUIRE_FALSE(w0.IsHiddenBehindDialog());
    REQUIRE_FALSE(w1.IsHiddenBehindDialog());
    REQUIRE_FALSE(w2.IsHiddenBehindDialog());
    REQUIRE_FALSE(w3.IsHiddenBehindDialog());

    //check linked list
    LinkedListCheck(popup_cnt, dialog_cnt, strong_dialog_cnt);
}

Rect16 MockScreen::GetInvalidationRect() const {
    return getInvalidationRect();
}

void MockScreen::checkPtrRange(window_t *&iter, size_t cnt, window_t *first, window_t *last) const {
    REQUIRE_FALSE(iter == nullptr);
    if (cnt) {
        //not empty list
        REQUIRE_FALSE(first == nullptr);
        REQUIRE_FALSE(last == nullptr);
        REQUIRE(iter->GetNext() == first);
        while (cnt--) {
            iter = iter->GetNext();
            REQUIRE_FALSE(iter == nullptr);
        }
        REQUIRE(iter == last);
    } else {
        //empty list
        REQUIRE(first == nullptr);
        REQUIRE(last == nullptr);
    }
}

window_dlg_strong_warning_t::window_dlg_strong_warning_t()
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreen, IDialog::IsStrong::yes) {
}

void window_dlg_strong_warning_t::setIcon(int16_t resId) {
}

void window_dlg_strong_warning_t::show(string_view_utf8 txt) {
    if (!GetParent()) {
        window_t *parent = Screens::Access()->Get();
        if (parent) {
            parent->RegisterSubWin(*this);
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
    dlg.show(string_view_utf8::MakeNULLSTR());
}

void window_dlg_strong_warning_t::ShowPrintFan() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(string_view_utf8::MakeNULLSTR());
}

void window_dlg_strong_warning_t::ShowHeaterTimeout() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(string_view_utf8::MakeNULLSTR());
}

void window_dlg_strong_warning_t::ShowUSBFlashDisk() {
    static window_dlg_strong_warning_t dlg;
    dlg.show(string_view_utf8::MakeNULLSTR());
}
