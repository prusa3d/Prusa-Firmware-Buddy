// liveadjustz.hpp
#pragma once

#include "IDialog.hpp"
#include "window_text.hpp"
#include "window_numb.hpp"
#include "window_icon.hpp"
#include "window_arrows.hpp"
#include "../../lang/i18n.h"

//regular window binded to Z calib
class WindowLiveAdjustZ : public AddSuperWindow<window_frame_t> {
    window_numb_t number;
    WindowArrows arrows;

public:
    WindowLiveAdjustZ(window_t *parent, point_i16_t pt);
    void Save();
    virtual ~WindowLiveAdjustZ() override { Save(); }
    float GetValue() const { return number.GetValue(); }

protected:
    void Change(int dif);

    static constexpr Rect16 getNumberRect(point_i16_t pt) {
        return Rect16(pt, 80, 25);
    }
    static constexpr point_i16_t getIconPoint(point_i16_t pt) {
        point_i16_t ret = getNumberRect(pt).TopEndPoint();
        return { ret.x, int16_t(ret.y + 5) };
    }

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class LiveAdjustZ : public AddSuperWindow<IDialog> {
    window_text_t text;
    window_icon_t nozzle_icon;
    window_frame_t bed;
    WindowLiveAdjustZ adjuster;

    LiveAdjustZ(is_closed_on_click_t outside_close); // created by static Open method

public:
    static void Open(is_closed_on_click_t outside_close = is_closed_on_click_t::yes);

protected:
    void moveNozzle();

    const Rect16 getTextRect();
    const Rect16 getNozzleRect();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
