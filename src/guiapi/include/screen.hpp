/**
 * @file screen.hpp
 * @author Radek Vana
 * @brief Similar to frame but can have dialogs.
 * @date 2020-11-29
 */

#pragma once

#include "window_frame.hpp"

class screen_t : public AddSuperWindow<window_frame_t> {
    window_t *captured_normal_window; //might need to move it in window frame after menu refactoring

    window_t *first_dialog;
    window_t *last_dialog;

    window_t *first_strong_dialog;
    window_t *last_strong_dialog;

    window_t *first_popup;
    window_t *last_popup;

public:
    screen_t(window_t *parent = nullptr, Rect16 rect = GuiDefaults::RectScreen, win_type_t type = win_type_t::normal, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_serial_t serial = is_closed_on_serial_t::yes);
    bool CaptureNormalWindow(window_t &win);
    void ReleaseCaptureOfNormalWindow();
    bool IsChildCaptured() const;
    virtual window_t *GetCapturedWindow() override;

protected:
    virtual bool registerSubWin(window_t &win) override;
    virtual void unregisterSubWin(window_t &win) override;

    void unregisterConflictingPopUps(Rect16 rect, window_t *end);
    bool canRegisterPopup(window_t &win);
    void hideSubwinsBehindDialogs();

    window_t *getCapturedNormalWin() const;

    window_t *getFirstDialog() const;
    window_t *getLastDialog() const;

    window_t *getFirstStrongDialog() const;
    window_t *getLastStrongDialog() const;

    window_t *getFirstPopUp() const;
    window_t *getLastPopUp() const;
};
