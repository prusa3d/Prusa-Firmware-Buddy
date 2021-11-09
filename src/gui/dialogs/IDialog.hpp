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
public:
    enum class IsStrong : bool { no,
        yes };
    IDialog(Rect16 rc = GuiDefaults::DialogFrameRect, IsStrong strong = IsStrong::no);
    IDialog(window_t *parent, Rect16 rc = GuiDefaults::DialogFrameRect);

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

protected:
    //used in MakeBlocking
    //needs included files which cannot be included in header
    bool consumeCloseFlag() const;
    void guiLoop() const;
};

void create_blocking_dialog_from_normal_window(window_t &dlg);
void create_blocking_dialog(IDialog &dlg);
