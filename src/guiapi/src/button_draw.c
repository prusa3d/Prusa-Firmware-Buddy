// button_draw.c
#include "button_draw.h"
#include "display_helper.h" //render_text_align

void button_draw(rect_ui16_t rc_btn, const char *text, const font_t *pf, bool is_selected) {
    color_t back_cl = is_selected ? COLOR_ORANGE : COLOR_GRAY;
    color_t text_cl = is_selected ? COLOR_BLACK : COLOR_WHITE;
    render_text_align(rc_btn, text, pf, back_cl, text_cl, padding_ui8(0, 0, 0, 0), ALIGN_CENTER); //@@TODO translate text here?
}
