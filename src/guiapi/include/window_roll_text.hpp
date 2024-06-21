/*  window_roll_text.hpp
 *   \brief used in texts that are too long for standart display width
 *
 *  Created on: May 6, 2020
 *      Author: Migi - michal.rudolf<at>prusa3d.cz
 */

#pragma once

#include "window_text.hpp"
#include "display_helper.h" //txtroll_t
#include "text_roll.hpp"

class window_roll_text_t : public window_text_t {
    txtroll_t roll;

    void rollInit() { roll.Init(GetRect(), text, get_font(), padding, GetAlignment()); }

public:
    window_roll_text_t(window_t *parent, Rect16 rect, const string_view_utf8 &txt = string_view_utf8::MakeNULLSTR(), Align_t align = GuiDefaults::Align());
    virtual void SetText(const string_view_utf8 &txt) override;
    /**
     * @brief Sets the Rect
     *
     * @return true if a change was done, false if no change
     */
    bool SetRect(Rect16 rect);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
