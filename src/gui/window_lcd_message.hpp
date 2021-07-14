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

#pragma once

#include "window_text.hpp"

#define LCD_MESSAGE_MAX_LEN 30
extern char lcd_message_text[LCD_MESSAGE_MAX_LEN + 1];

class WindowLCDMessage : public AddSuperWindow<window_text_t> {
    char last_lcd_message_text[LCD_MESSAGE_MAX_LEN + 1];

public:
    WindowLCDMessage(window_t *parent, Rect16 rect);

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
