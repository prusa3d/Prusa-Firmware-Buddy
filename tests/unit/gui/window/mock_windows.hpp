/**
 * @file mock_windows.hpp
 * @author Radek Vana
 * @brief Mock windows and screens to check window registration and capture
 * @date 2020-11-30
 */

#pragma once
#include "catch2/catch.hpp"
#include "screen.hpp"
#include "window_dlg_popup.hpp"
#include "IDialog.hpp"
#include "DialogTimed.hpp"
#include "guitypes.hpp"

class window_dlg_strong_warning_t : public AddSuperWindow<IDialog> {
protected: // inherited by unit tests, must be protected
    window_dlg_strong_warning_t();
    window_dlg_strong_warning_t(const window_dlg_strong_warning_t &) = delete;

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
    void show(string_view_utf8 txt); // could use const char *, but with stringview I can pass both translated and not translated texts
    void setIcon(const img::Resource *res);

public:
    static void ShowHotendFan();
    static void ShowPrintFan();
    static void ShowHeaterTimeout();
    static void ShowUSBFlashDisk();
};

struct MockFrame_VisibilityNotifycations : public AddSuperWindow<window_frame_t> {
    window_t win;
    uint32_t ChangedCounter;
    virtual void ChildVisibilityChanged(window_t &child) override {
        super::ChildVisibilityChanged(child);
        ++ChangedCounter;
    }

    MockFrame_VisibilityNotifycations()
        : win(this, Rect16(20, 20, 10, 10))
        , ChangedCounter(0) {}

    Rect16 GetInvRect() const { return getInvalidationRect(); }
};

struct MockMsgBox : public AddSuperWindow<IDialog> {
    MockMsgBox(Rect16 rc)
        : AddSuperWindow<IDialog>(rc) {}
};

class MockStrongDialog : public AddSuperWindow<window_dlg_strong_warning_t> {
public:
    void Show(string_view_utf8 txt) { show(txt); }

    static MockStrongDialog &ShowHotendFan() {
        static MockStrongDialog dlg;
        dlg.Show(string_view_utf8::MakeNULLSTR());
        return dlg;
    }

    static MockStrongDialog &ShowPrintFan() {
        static MockStrongDialog dlg;
        dlg.Show(string_view_utf8::MakeNULLSTR());
        return dlg;
    }

    static MockStrongDialog &ShowHeatersTimeout() {
        static MockStrongDialog dlg;
        dlg.Show(string_view_utf8::MakeNULLSTR());
        return dlg;
    }

    static MockStrongDialog &ShowUSBFlashDisk() {
        static MockStrongDialog dlg;
        dlg.Show(string_view_utf8::MakeNULLSTR());
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

    void ParrentCheck() const;
    void LinkedListCheck(size_t popup_cnt = 0, size_t dialog_cnt = 0, size_t strong_dialog_cnt = 0) const;
    void BasicCheck(size_t popup_cnt = 0, size_t dialog_cnt = 0, size_t strong_dialog_cnt = 0) const;

    template <class... E>
    void CheckOrderAndVisibility(E *...e);

    Rect16 GetInvalidationRect() const;
    Rect16 GetInvRect() const { return getInvalidationRect(); }

private:
    void checkPtrRange(window_t *&iter, size_t cnt, window_t *first, window_t *last) const;

    template <class T>
    static void checkHidden(const T &arr, window_t &win);
};

template <class T>
void MockScreen::checkHidden(const T &extra_windows, window_t &win) {
    bool hidden = false;

    for (size_t i = 0; i < extra_windows.size(); ++i) {
        if (win.GetRect().HasIntersection(extra_windows[i]->GetRect())) {
            hidden = true;
        }
    }

    // check IsHiddenBehindDialog()
    REQUIRE(win.IsHiddenBehindDialog() == hidden);
}

template <class... E>
void MockScreen::CheckOrderAndVisibility(E *...e) {
    constexpr size_t sz = sizeof...(e);
    std::array<window_t *, sz> extra_windows = { e... };

    size_t popup_cnt = 0;
    size_t dialog_cnt = 0;
    size_t strong_dialog_cnt = 0;

    for (size_t i = 0; i < sz; ++i) {
        switch (extra_windows[i]->GetType()) {
        case win_type_t::dialog:
            ++dialog_cnt;
            break;
        case win_type_t::strong_dialog:
            ++strong_dialog_cnt;
            break;
        case win_type_t::popup:
            ++popup_cnt;
            break;
        default:
            break;
        }
    }

    // check parrent
    ParrentCheck();

    // check linked list
    LinkedListCheck(popup_cnt, dialog_cnt, strong_dialog_cnt);

    // hidden check of normal windows
    REQUIRE_FALSE(getFirstNormal() == nullptr);
    REQUIRE_FALSE(getLastNormal() == nullptr);
    for (window_t *pWin = getFirstNormal(); pWin != getLastNormal()->GetNext(); pWin = pWin->GetNext()) {
        REQUIRE_FALSE(pWin == nullptr);
        checkHidden(extra_windows, *pWin);
    }

    // check hidden of extra_windows
    std::array<bool, sz> hiddens;
    hiddens.fill(false);
    for (int top_win_index = sz - 1; top_win_index >= 0; --top_win_index) {
        for (int bot_win_index = top_win_index - 1; bot_win_index >= 0; --bot_win_index) {
            if (extra_windows[top_win_index]->GetRect().HasIntersection(extra_windows[bot_win_index]->GetRect())) {
                hiddens[bot_win_index] = true;
            }
        }
        // outer loop can also check result of current window
        REQUIRE(extra_windows[top_win_index]->IsHiddenBehindDialog() == hiddens[top_win_index]);
    }

    window_t *pWin = &w_last;
    REQUIRE_FALSE(pWin == nullptr); // should never fail

    // check order of all extra windows
    for (size_t i = 0; i < sz; ++i) {
        pWin = pWin->GetNext();
        REQUIRE_FALSE(pWin == nullptr);
    }

    REQUIRE(pWin->GetNext() == nullptr); // verify if all windows were checked
}

class MockDialogTimed : public AddSuperWindow<DialogTimed> {

public:
    MockDialogTimed(window_t *parent, Rect16 rc, uint32_t time = 500)
        : AddSuperWindow<DialogTimed>(parent, rc, time) {
    }

protected:
    virtual void updateLoop(visibility_changed_t visibility_changed) override {};
};
