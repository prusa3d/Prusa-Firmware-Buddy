// IDialog.hpp
#pragma once

#include <stdint.h>
#include "window_frame.hpp"
#include "guitypes.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include <functional>

// interface for dialog
class IDialog : public AddSuperWindow<window_frame_t> {
public:
    IDialog(Rect16 rc = GuiDefaults::DialogFrameRect);
    IDialog(window_t *parent, Rect16 rc = GuiDefaults::DialogFrameRect);

public:
    // could be static, but I want it to be usable only from dialog
    void MakeBlocking(std::function<void()> loopCallback = {}) const;

protected:
    // used in MakeBlocking
    // needs included files which cannot be included in header
    bool consumeCloseFlag() const;
    void guiLoop() const;
};
