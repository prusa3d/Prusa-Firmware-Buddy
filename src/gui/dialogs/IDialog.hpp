//IDialog.hpp
#pragma once

#include <stdint.h>
#include "window_frame.hpp"
#include "guitypes.hpp"
#include "GuiDefaults.hpp"
#include <functional>

//todo remove this after jogwheel refactoring
extern void gui_loop(void);

//interface for dialog
class IDialog : public AddSuperWindow<window_frame_t> {
    window_t *prev_capture;

public:
    IDialog(Rect16 rc = GuiDefaults::RectScreenBody);
    virtual ~IDialog();

    static constexpr Rect16 get_radio_button_rect(Rect16 rc_frame) {
        return Rect16(
            rc_frame.Left() + GuiDefaults::ButtonSpacing,
            rc_frame.Top() + (rc_frame.Height() - GuiDefaults::ButtonHeight - GuiDefaults::FrameWidth),
            rc_frame.Width() - 2 * GuiDefaults::ButtonSpacing,
            GuiDefaults::ButtonHeight);
    }

    template <class... Args>
    void MakeBlocking(
        std::function<void(Args...)> action = [](Args...) {}, Args... args) const { //could be static, but I want it to be usable only from dialog
        while (!consumeCloseFlag()) {
            guiLoop();
            action(args...);
        }
    }

    window_t *GetStoredCapture() const { return prev_capture; }
    void StoreCapture();                         // set capture pointer (to be restore after dialog closes)
    void ModifyStoredCapture(window_t *capture); // in some cases another closing dialog can pass its capture
protected:
    void releaseCapture();
    void clearCapture();

    //used in MakeBlocking
    //needs included files which cannot be included in header
    bool consumeCloseFlag() const;
    void guiLoop() const;
};

void create_blocking_dialog_from_normal_window(window_t &dlg);
void create_blocking_dialog(IDialog &dlg);

class WinFilterDialogCapture : public WinFilter {
    window_t *target_capture;

public:
    constexpr WinFilterDialogCapture(window_t *capture)
        : target_capture(capture) {}

    virtual bool operator()(const window_t &win) const override { return win.IsDialog() && reinterpret_cast<const IDialog &>(win).GetStoredCapture() == target_capture; };
};
