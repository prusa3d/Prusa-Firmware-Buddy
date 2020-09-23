// liveadjustz.hpp
#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "window_icon.hpp"
#include "window_arrows.hpp"
#include "../../lang/i18n.h"

class LiveAdjustZ : public IDialog {
protected:
    window_text_t text;
    window_numb_t number;
    window_icon_t nozzle_icon;
    window_frame_t bed;
    WindowArrows arrows;

    LiveAdjustZ(Rect16 rect, is_closed_on_click_t outside_close);

public:
    void SaveAndClose();
    static void Open(Rect16 rect = GuiDefaults::RectScreenBody, is_closed_on_click_t outside_close = is_closed_on_click_t::yes);

protected:
    void Change(int dif);
    void moveNozzle();

    const Rect16 getTextRect();
    const Rect16 getNumberRect();
    const Rect16 getNozzleRect();

    virtual void windowEvent(window_t *sender, uint8_t event, void *param) override;
};
