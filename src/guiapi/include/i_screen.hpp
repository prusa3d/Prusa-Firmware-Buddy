/**
 * @file i_screen.hpp
 * @brief parent if screen
 * Similar to frame but can have dialogs.
 */

#pragma once

#include "window_frame.hpp"
#include "screen_init_variant.hpp"

// DO NOT SET has_relative_subwins flag !!!
// screen must have rect == GuiDefaults::RectScreen
class IScreen : public AddSuperWindow<window_frame_t> {
    window_t *first_dialog;
    window_t *last_dialog;

    window_t *first_strong_dialog;
    window_t *last_strong_dialog;

    window_t *first_popup;
    window_t *last_popup;

public:
    IScreen(window_t *parent, win_type_t type, is_closed_on_timeout_t timeout, is_closed_on_serial_t serial);

    virtual window_t *GetCapturedWindow() override;
    virtual void ChildVisibilityChanged(window_t &child) override;

    virtual window_t *GetFirstDialog() const override;
    virtual window_t *GetLastDialog() const override;

    virtual window_t *GetFirstStrongDialog() const override;
    virtual window_t *GetLastStrongDialog() const override;

    virtual window_t *GetFirstPopUp() const override;
    virtual window_t *GetLastPopUp() const override;

    virtual void InitState(screen_init_variant var) {}
    virtual screen_init_variant GetCurrentState() const { return screen_init_variant(); }

protected:
    virtual bool registerSubWin(window_t &win) override;
    virtual void unregisterSubWin(window_t &win) override;

    void unregisterConflictingPopUps(Rect16 rect, window_t *end);
    bool canRegisterPopup(window_t &win);
    void hideSubwinsBehindDialogs();
    window_t *findCaptured_first_last(window_t *first, window_t *last) const; //does not use begin - end like normal find
};
