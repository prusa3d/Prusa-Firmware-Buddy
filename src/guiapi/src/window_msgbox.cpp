// window_msgbox.c
#include "window_msgbox.h"
#include "gui.h"
#include "resource.h"
#include "button_draw.h"
#include "sound_C_wrapper.h"
#include "../lang/i18n.h"
#include <algorithm>

//title for each icon type (empty text for 0)
const char *window_msgbox_title_text[] = {
    "",            // MSGBOX_ICO_CUSTOM     0x0000
    "Error",       // MSGBOX_ICO_ERROR      0x0010
    "Question",    // MSGBOX_ICO_QUESTION   0x0020
    "Warning",     // MSGBOX_ICO_WARNING    0x0030
    "Information", // MSGBOX_ICO_INFO       0x0040
};

//number of buttons for each button configuration
const uint8_t window_msgbox_button_count[] = {
    1, // MSGBOX_BTN_OK               0x0000
    2, // MSGBOX_BTN_OKCANCEL         0x0001
    3, // MSGBOX_BTN_ABORTRETRYIGNORE 0x0002
    3, // MSGBOX_BTN_YESNOCANCEL      0x0003
    2, // MSGBOX_BTN_YESNO            0x0004
    2, // MSGBOX_BTN_RETRYCANCEL      0x0005
    1, // MSGBOX_BTN_CUSTOM1          0x0006
    2, // MSGBOX_BTN_CUSTOM2          0x0007
    3, // MSGBOX_BTN_CUSTOM3          0x0008
};

//button types in each button configuration (0 means "no button")
const uint8_t window_msgbox_buttons[][3] = {
    { MSGBOX_RES_OK, 0, 0 },                                        // MSGBOX_BTN_OK
    { MSGBOX_RES_OK, MSGBOX_RES_CANCEL, 0 },                        // MSGBOX_BTN_OKCANCEL
    { MSGBOX_RES_ABORT, MSGBOX_RES_RETRY, MSGBOX_RES_IGNORE },      // MSGBOX_BTN_ABORTRETRYIGNORE
    { MSGBOX_RES_YES, MSGBOX_RES_NO, MSGBOX_RES_CANCEL },           // MSGBOX_BTN_YESNOCANCEL
    { MSGBOX_RES_YES, MSGBOX_RES_NO, 0 },                           // MSGBOX_BTN_YESNO
    { MSGBOX_RES_RETRY, MSGBOX_RES_CANCEL, 0 },                     // MSGBOX_BTN_RETRYCANCEL
    { MSGBOX_RES_CUSTOM0, 0, 0 },                                   // MSGBOX_BTN_CUSTOM1
    { MSGBOX_RES_CUSTOM0, MSGBOX_RES_CUSTOM1, 0 },                  // MSGBOX_BTN_CUSTOM2
    { MSGBOX_RES_CUSTOM0, MSGBOX_RES_CUSTOM1, MSGBOX_RES_CUSTOM2 }, // MSGBOX_BTN_CUSTOM3
};

//button text for each button type (empty text for 0, 1 and 9)
const char *window_msgbox_button_text[] = {
    "",          //                      0
    "",          //                      1
    "CANCEL",    // MSGBOX_RES_CANCEL    2
    "ABORT",     // MSGBOX_RES_ABORT     3
    "RETRY",     // MSGBOX_RES_RETRY     4
    "IGNORE",    // MSGBOX_RES_IGNORE    5
    "YES",       // MSGBOX_RES_YES       6
    "NO",        // MSGBOX_RES_NO        7
    "OK",        // MSGBOX_RES_OK        8
    "",          //                      9
    "TRY AGAIN", // MSGBOX_RES_TRYAGAIN 10
    "CONTINUE",  // MSGBOX_RES_CONTINUE 11
    "CUSTOM0",   // MSGBOX_RES_CUSTOM0  12
    "CUSTOM1",   // MSGBOX_RES_CUSTOM1  13
    "CUSTOM2",   // MSGBOX_RES_CUSTOM2  14
};

//icon ids - null by defult
uint16_t window_msgbox_id_icon[5] = {
    0, // custom
    0, // error
    0, // question
    0, // warning
    0, // info
};

const padding_ui8_t window_padding = { 8, 2, 8, 2 }; // left, top, right, bottom

/// Draws window's buttons at the bottom
void window_msgbox_draw_buttons(window_msgbox_t *window) {
    rect_ui16_t rc_btn = window->win.rect;
    rc_btn.y += (rc_btn.h - gui_defaults.btn_h - gui_defaults.frame_width);
    rc_btn.h = gui_defaults.btn_h;

    const int btn = ((window->flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    if (btn > MSGBOX_BTN_MAX)
        return;                                                           //invalid config - exit
    const int count = window_msgbox_button_count[btn];                    // get number of buttons from table
    const uint8_t *buttons = window_msgbox_buttons[btn];                  // get pointer to 3 element button array
    const int idx = ((window->flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX); // selected button index
    const int chg = ((window->flags & MSGBOX_MSK_CHG) >> MSGBOX_SHI_CHG); // change mask
    if (chg == 7)
        display::FillRect(rc_btn, window->color_back);             //clear background if all buttons changed
    const int spacing2 = gui_defaults.btn_spacing;                 // button spacing
    const int btn_w = (rc_btn.w - (count * 2 * spacing2)) / count; // avg width of a button
    const font_t *pf = window->font_title;
    float chars = 0; // average number of chars in a button

    for (int i = 0; i < count; i++) {
        if (window->buttons[i] == NULL) // set default button in case of missing one
            window->buttons[i] = window_msgbox_button_text[buttons[i]];
        chars += strlen(window->buttons[i]);
    }

    chars /= count;
    rc_btn.x += spacing2;

    for (int i = 0; i < count; i++) {
        rc_btn.w = btn_w + pf->w * (strlen(window->buttons[i]) - chars);
        if (chg & (1 << i)) {
            button_draw(rc_btn, window->buttons[i], pf, i == idx);
        }
        rc_btn.x += rc_btn.w + 2 * spacing2; // next button is 2x spacing to the right
    }

    window->flags &= ~MSGBOX_MSK_CHG;
}

/// Actions after knob has turned
void window_msgbox_step(window_msgbox_t *window, int step) {
    const int btn = ((window->flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    const int count = window_msgbox_button_count[btn];                    // get number of buttons from table
    int idx = ((window->flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX);       // selected button index
    int chg = (1 << idx);                                                 // change mask - old button
    idx += step;                                                          // increment index
    if (idx < 0) {
        idx = 0; // check min
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    if (idx >= count) {
        idx = count - 1; // check max
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    chg |= (1 << idx);                                  // change mask - new button
    window->flags = (window->flags & ~MSGBOX_MSK_IDX) | // clear index bits
        ((idx << MSGBOX_SHI_IDX) & MSGBOX_MSK_IDX) |    // set new index bits
        ((chg << MSGBOX_SHI_CHG) & MSGBOX_MSK_CHG);     // set change flags
    gui_invalidate();
}

/// Actions after knob was pushed
void window_msgbox_click(window_msgbox_t *window) {
    const int btn = ((window->flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    const int idx = ((window->flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX); // selected button index
    window->res = window_msgbox_buttons[btn][idx];
    Sound_Stop();
    window_destroy(window->win.id);
}

/// Init message box to default values
void window_msgbox_init(window_msgbox_t *window) {
    if (rect_empty_ui16(window->win.rect)) //use display rect if current rect is empty
        window->win.rect = rect_ui16(0, 0, display::GetW(), display::GetH());
    window->win.flg |= WINDOW_FLG_ENABLED; //enabled by default
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = window_padding;
    window->alignment = ALIGN_CENTER;
    window->title = 0;
    window->id_icon = 0;
    window->text = 0;
    window->flags = MSGBOX_BTN_OK | MSGBOX_ICO_INFO;
    window->res = 0;
}

void window_msgbox_done(window_msgbox_t *window) {
}

/// Draws parts of message box that require redraw
void window_msgbox_draw(window_msgbox_t *window) {
    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        display::FillRect(window->win.rect, COLOR_BLACK); // clear window

        uint8_t red_line_offset = 0;
        const int ico = ((window->flags & MSGBOX_MSK_ICO) >> MSGBOX_SHI_ICO);
        // get title from window member or set default (info, warning, error...)
        const char *title = (window->title != NULL) ? window->title : window_msgbox_title_text[ico];
        const int title_n = strlen(title); // number of chars in title
        // title height in pixels; if not empty, use font height
        int title_h = (!title_n) ? 0 : window->font_title->h;

        // get icon id from window member; for error, warning, info and question -> disable icon
        const uint16_t id_icon = (ico < 1 && window->id_icon == 0) ? window_msgbox_id_icon[ico] : window->id_icon;
        size_ui16_t icon_dim = size_ui16(0, 0);
        const uint8_t *p_icon = 0;                             // icon resource pointer
        if (id_icon && (p_icon = resource_ptr(id_icon))) {     // id_icon is set and resource pointer is not null
            icon_dim = icon_size(p_icon);                      // get icon dimensions
            title_h = std::max(uint16_t(title_h), icon_dim.h); // adjust title height
        }

        if (title_h) {                                               // render visible text only (title_h > 0)
            title_h += window->padding.top + window->padding.bottom; // add padding
            rect_ui16_t rc_tit = window->win.rect;
            rc_tit.h = title_h;      // xxx pixels for title
            if (title_n && p_icon) { // text and icon available => all will be aligned left
                const int icon_w = icon_dim.w + window->padding.left + window->padding.right;
                rc_tit.w = icon_w;
                render_icon_align(rc_tit, id_icon, window->color_back, ALIGN_CENTER);
                rc_tit.x = icon_w;
                rc_tit.w = window->win.rect.w - icon_w;
                render_text_align(rc_tit, _(title), window->font_title, window->color_back, window->color_text, window->padding, ALIGN_LEFT_CENTER);
            } else if (title_n) { // text not empty but no icon => text will be aligned left
                render_text_align(rc_tit, _(title), window->font_title, window->color_back, window->color_text, window->padding, ALIGN_LEFT_CENTER);
                display::DrawLine(point_ui16(rc_tit.x + window->padding.left, rc_tit.y + rc_tit.h),
                    point_ui16(rc_tit.x + rc_tit.w - (window->padding.left + window->padding.right), rc_tit.y + rc_tit.h),
                    COLOR_RED_ALERT);
                red_line_offset = 1;
            } else { // no text but icon available => icon will be aligned to center
                render_icon_align(rc_tit, id_icon, window->color_back, ALIGN_CENTER);
            }
        }

        const rect_ui16_t rc_txt = { window->win.rect.x,
            uint16_t(window->win.rect.y + title_h + red_line_offset), // put text bellow title and red line
            window->win.rect.w,
            uint16_t(window->win.rect.h - (title_h + red_line_offset + gui_defaults.btn_h)) };
        render_text_align(rc_txt, _(window->text), window->font, window->color_back, window->color_text, window->padding, window->alignment | RENDER_FLG_WORDB);

        window->flags |= MSGBOX_MSK_CHG;
        window_msgbox_draw_buttons(window);

        window->win.flg &= ~WINDOW_FLG_INVALID;
    } else if (window->flags & MSGBOX_MSK_CHG)
        window_msgbox_draw_buttons(window);
    if (window->flags & MSGBOX_GREY_FRAME) {                                /// draw frame
        const uint16_t w = (display::GetW() - 1) - window->win.rect.x + 1;  /// last - first + 1
        const uint16_t h = (display::GetH() - 67) - window->win.rect.y + 1; /// last - first + 1
        display::DrawRect(rect_ui16(window->win.rect.x, window->win.rect.y, w, h), COLOR_GRAY);
    }
}

void window_msgbox_event(window_msgbox_t *window, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        window_msgbox_click(window);
        break;
    case WINDOW_EVENT_ENC_DN:
        window_msgbox_step(window, -1);
        break;
    case WINDOW_EVENT_ENC_UP:
        window_msgbox_step(window, +1);
        break;
    }
}

const window_class_msgbox_t window_class_msgbox = {
    {
        WINDOW_CLS_MSGBOX,
        sizeof(window_msgbox_t),
        (window_init_t *)window_msgbox_init,
        (window_done_t *)window_msgbox_done,
        (window_draw_t *)window_msgbox_draw,
        (window_event_t *)window_msgbox_event,
    },
};
