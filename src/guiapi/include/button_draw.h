// button_draw.h
#pragma once

#include "stdbool.h"
#include "guitypes.h"
#include "../../lang/string_view_utf8.hpp"

extern void button_draw(rect_ui16_t rc_btn, string_view_utf8 text, const font_t *pf, bool is_selected);
