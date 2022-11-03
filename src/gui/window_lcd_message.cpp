/*
 * Copyright (c) 2021 Matthew Lloyd <github@matthewlloyd.net>
 * All rights reserved.
 *
 * This file is part of Llama Mini.
 *
 * Llama Mini is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Llama Mini is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Llama Mini. If not, see <https://www.gnu.org/licenses/>.
 */

#include "window_lcd_message.hpp"
#include "gui.hpp"
#include <algorithm>
#include "resource.h"
#include "marlin_client.h"

char lcd_message_text[LCD_MESSAGE_MAX_LEN + 1];

/*****************************************************************************/
//WindowLCDMessage
WindowLCDMessage::WindowLCDMessage(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_text_t>(parent, rect, is_multiline::no) {
    last_lcd_message_text[0] = 0;
    last_lcd_message_text[LCD_MESSAGE_MAX_LEN] = 0;
    lcd_message_text[0] = 0;
    lcd_message_text[LCD_MESSAGE_MAX_LEN] = 0;
    font = resource_font(IDR_FNT_SMALL);
    SetAlignment(Align_t::Center());
}

void WindowLCDMessage::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        if (strncmp(lcd_message_text, last_lcd_message_text, LCD_MESSAGE_MAX_LEN) != 0) {
            strncpy(last_lcd_message_text, lcd_message_text, LCD_MESSAGE_MAX_LEN);
            SetText(string_view_utf8::MakeRAM((const uint8_t *)lcd_message_text));
            Invalidate();
        }
    }
    SuperWindowEvent(sender, event, param);
}
