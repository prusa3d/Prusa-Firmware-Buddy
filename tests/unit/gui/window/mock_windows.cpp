/**
 * @file mock_windows.cpp
 * @author Radek Vana
 * @date 2020-11-30
 */
#include "mock_windows.hpp"
#include "catch2/catch.hpp"

void MockScreen::ParrentCheck() {
    //check parrent
    REQUIRE(w_first.GetParent() == this);
    REQUIRE(w_last.GetParent() == this);
    REQUIRE(w0.GetParent() == this);
    REQUIRE(w1.GetParent() == this);
    REQUIRE(w2.GetParent() == this);
    REQUIRE(w3.GetParent() == this);
}

void MockScreen::LinkedListCheck(size_t popup_cnt, size_t dialog_cnt, size_t strong_dialog_cnt) {
    //check linked list
    REQUIRE(getFirstNormal() == &(w_first));
    REQUIRE(getLastNormal() == &(w_last));
    REQUIRE(getFirstNormal()->GetNext() == &(w0));
    REQUIRE(w0.GetNext() == &(w1));
    REQUIRE(w1.GetNext() == &(w2));
    REQUIRE(w2.GetNext() == &(w3));
    REQUIRE(w3.GetNext() == &(w_last));

    window_t *pLast = getLastNormal();
    REQUIRE_FALSE(pLast == nullptr);

    /*  size_t cnt = popup_cnt + dialog_cnt + strong_dialog_cnt;
    for (size_t i = 0; i < cnt; ++i ) {
        pLast = pLast->GetNext();
        REQUIRE_FALSE(pLast == nullptr);
    }*/

    if (dialog_cnt) {
        REQUIRE(pLast->GetNext() == nullptr);
        while (dialog_cnt--) {
            pLast = pLast->GetNext();
            REQUIRE_FALSE(pLast == nullptr);
        }
    }

    while (strong_dialog_cnt--) {
        pLast = pLast->GetNext();
        REQUIRE_FALSE(pLast == nullptr);
    }

    while (popup_cnt--) {
        pLast = pLast->GetNext();
        REQUIRE_FALSE(pLast == nullptr);
    }

    pLast = pLast->GetNext();
    REQUIRE(pLast == nullptr);
}

void MockScreen::BasicCheck(size_t popup_cnt, size_t dialog_cnt, size_t strong_dialog_cnt) {
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
/*
template <class... E>
void check_window_order_and_visibility(MockScreen &screen, E *... e) {
    constexpr size_t sz = sizeof...(e);
    std::array<window_t *, sz> extra_windows = { e... };

    //check parrent
    screen.ParrentCheck();

    //check IsHiddenBehindDialog()
    REQUIRE_FALSE(screen.w_first.IsHiddenBehindDialog());
    REQUIRE_FALSE(screen.w_last.IsHiddenBehindDialog());
    REQUIRE(screen.w0.IsHiddenBehindDialog());
    REQUIRE(screen.w1.IsHiddenBehindDialog());
    REQUIRE(screen.w2.IsHiddenBehindDialog());
    REQUIRE(screen.w3.IsHiddenBehindDialog());

    //check linked list
    screen.LinkedListCheck(has_dialog_t::yes);

    //check last pointer
    REQUIRE(screen.getLastNormal() == extra_windows[sz - 1]);

    window_t *pWin = &screen.w_last;
    REQUIRE_FALSE(pWin == nullptr); //should never fail

    //check order of all extra windows
    for (size_t i = 0; i < sz; ++i) {
        pWin = pWin->GetNext();
        REQUIRE_FALSE(pWin == nullptr);
    }

    REQUIRE(pWin == screen.getLastNormal()); // check if only 1 extra window is registered
}
*/
