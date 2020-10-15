/*  window_roll_text.hpp
*   \brief used in texts that are too long for standart display width
*
*  Created on: May 6, 2020
*      Author: Migi - michal.rudolf<at>prusa3d.cz
*/

#pragma once

#include "window_text.hpp"
#include "display_helper.h" //txtroll_t

class window_roll_text_t : public AddSuperWindow<window_text_t> {
    txtroll_t roll;

    bool rollNeedInit() { return roll.setup == TXTROLL_SETUP_INIT; }
    void rollInit() { roll_init(rect, text, font, padding, GetAlignment(), &roll); }

public:
    window_roll_text_t(window_t *parent, Rect16 rect, string_view_utf8 txt);
    virtual ~window_roll_text_t();
    virtual void SetText(string_view_utf8 txt) override;

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
