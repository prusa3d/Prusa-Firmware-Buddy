// button_draw.h
#pragma once

#include "guitypes.hpp"
#include "../../lang/string_view_utf8.hpp"

extern void button_draw(Rect16 rc_btn, string_view_utf8 text, const font_t *pf, bool is_selected);
