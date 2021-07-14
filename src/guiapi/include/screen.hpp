/**
 * @file screen.hpp
 * @author Radek Vana
 * @brief Similar to frame but can have dialogs.
 * @date 2020-11-29
 */

#pragma once

#include "window_frame.hpp"

// DO NOT SET has_relative_subwins flag !!!
// screen must have rect == GuiDefaults::RectScreen
class screen_t : public AddSuperWindow<window_frame_t> {
    window_t *first_dialog;
    window_t *last_dialog;

    window_t *first_strong_dialog;
    window_t *last_strong_dialog;

    window_t *first_popup;
    window_t *last_popup;

public:
    screen_t(window_t *parent = nullptr, win_type_t type = win_type_t::normal, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_serial_t serial = is_closed_on_serial_t::yes);

    virtual window_t *GetCapturedWindow() override;
    virtual void ChildVisibilityChanged(window_t &child) override;

    virtual window_t *GetFirstDialog() const override;
    virtual window_t *GetLastDialog() const override;

    virtual window_t *GetFirstStrongDialog() const override;
    virtual window_t *GetLastStrongDialog() const override;

    virtual window_t *GetFirstPopUp() const override;
    virtual window_t *GetLastPopUp() const override;

protected:
    virtual bool registerSubWin(window_t &win) override;
    virtual void unregisterSubWin(window_t &win) override;

    void unregisterConflictingPopUps(Rect16 rect, window_t *end);
    bool canRegisterPopup(window_t &win);
    void hideSubwinsBehindDialogs();
    window_t *findCaptured_first_last(window_t *first, window_t *last) const; //does not use begin - end like normal find
};
