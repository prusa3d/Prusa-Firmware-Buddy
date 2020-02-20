// window_msgbox.c
#include "window_msgbox.h"
#include "gui.h"
#include "resource.h"
#include "button_draw.h"

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

void window_msgbox_draw_buttons(window_msgbox_t *window) {
    rect_ui16_t rc_btn = window->win.rect;
    rc_btn.y += (rc_btn.h - 40); // 30pixels for buttons + 10pix for grey frame
    rc_btn.h = 30;
    int btn = ((window->flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    if (btn > MSGBOX_BTN_MAX)
        return;                                                     //invalid config - exit
    int count = window_msgbox_button_count[btn];                    // get number of buttons from table
    const uint8_t *buttons = window_msgbox_buttons[btn];            // get pointer to 3 element button array
    int idx = ((window->flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX); // selected button index
    int chg = ((window->flags & MSGBOX_MSK_CHG) >> MSGBOX_SHI_CHG); // change mask
    if (chg == 7)                                                   //clear background if all buttons changed
        display->fill_rect(rc_btn, window->color_back);
    int spacing2 = gui_defaults.btn_spacing;                 // 12 pixels spacing between buttons, 6 from margins
    int btn_w = (rc_btn.w - (count * 2 * spacing2)) / count; // avg width of button
    int i;
    font_t *pf = window->font_title;
    float chars = 0;
    const char *text;
    for (i = 0; i < count; i++) {
        uint8_t b = buttons[i];
        text = window->buttons[i];
        if (text == 0)
            text = window_msgbox_button_text[b];
        chars += strlen(text);
    }
    chars /= count; //avg chars for button
    rc_btn.x += spacing2;
    for (i = 0; i < count; i++) {
        uint8_t b = buttons[i];
        text = window->buttons[i];
        if (text == 0)
            text = window_msgbox_button_text[b];
        rc_btn.w = btn_w + pf->w * ((float)strlen(text) - chars);
        if (chg & (1 << i)) {
            int is_selected = (i == idx) ? 1 : 0; //state of button (1=selected)
            button_draw(rc_btn, text, pf, is_selected);
        }
        rc_btn.x += rc_btn.w + 2 * spacing2;
    }
    window->flags &= ~MSGBOX_MSK_CHG;
}

void window_msgbox_step(window_msgbox_t *window, int step) {
    int btn = ((window->flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    int count = window_msgbox_button_count[btn];                    // get number of buttons from table
    int idx = ((window->flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX); // selected button index
    int chg = (1 << idx);                                           // change mask - old button
    idx += step;                                                    // increment index
    if (idx < 0)
        idx = 0; // check min
    if (idx >= count)
        idx = count - 1;                                // check max
    chg |= (1 << idx);                                  // change mask - new button
    window->flags = (window->flags & ~MSGBOX_MSK_IDX) | // clear index bits
        ((idx << MSGBOX_SHI_IDX) & MSGBOX_MSK_IDX) |    // set new index bits
        ((chg << MSGBOX_SHI_CHG) & MSGBOX_MSK_CHG);     // set change flags
    gui_invalidate();
}

void window_msgbox_click(window_msgbox_t *window) {
    int btn = ((window->flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    //int count = window_msgbox_button_count[btn]; // get number of buttons from table
    int idx = ((window->flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX); // selected button index
    window->res = window_msgbox_buttons[btn][idx];
    window_destroy(window->win.id);
}

void window_msgbox_init(window_msgbox_t *window) {
    if (rect_empty_ui16(window->win.rect)) //use display rect if current rect is empty
        window->win.rect = rect_ui16(0, 0, display->w, display->h);
    window->win.flg |= WINDOW_FLG_ENABLED; //enabled by default
    window->color_back = gui_defaults.color_back;
    window->color_text = gui_defaults.color_text;
    window->font = gui_defaults.font;
    window->font_title = gui_defaults.font_big;
    window->padding = padding_ui8(8, 2, 8, 2);
    window->alignment = ALIGN_CENTER;
    window->title = 0;
    window->id_icon = 0;
    window->text = 0;
    window->flags = MSGBOX_BTN_OK | MSGBOX_ICO_INFO;
    window->res = 0;
}

void window_msgbox_done(window_msgbox_t *window) {
}

void window_msgbox_draw(window_msgbox_t *window) {
    if (((window->win.flg & (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE)) == (WINDOW_FLG_INVALID | WINDOW_FLG_VISIBLE))) {
        rect_ui16_t rc_tit = window->win.rect;

        uint8_t red_line_offset = 0;
        int ico = ((window->flags & MSGBOX_MSK_ICO) >> MSGBOX_SHI_ICO);
        const char *title = window->title; // get title from window member
        if (title == 0)                    // if null, set defaut title (info, warning, error...)
            title = window_msgbox_title_text[ico];
        int title_h = 0;             // title hight in pixels
        int title_n = strlen(title); // number of chars in title
        if (title_n)                 // if not empty, set title hight from font
            title_h = window->font_title->h;
        uint16_t id_icon = window->id_icon; // get icon id from window member
        if (ico < 1) {                      // for error, warning, info and question -> disable icon
            if (id_icon == 0)
                id_icon = window_msgbox_id_icon[ico];
        }
        const uint8_t *picon = 0;                           // icon resource pointer
        point_ui16_t icon_wh = point_ui16(0, 0);            // icon width-height - default (0,0)
        if ((id_icon) && (picon = resource_ptr(id_icon))) { // id_icon is set and resource pointer is not null
            icon_wh = icon_meas(picon);                     // get icon dimensions
            if (title_h < icon_wh.y)
                title_h = icon_wh.y; // adjust title height
        }
        if (title_h) // calculated title height != 0 means title will be rendered
        {
            title_h += window->padding.top + window->padding.bottom; // add padding
            rc_tit.h = title_h;                                      // xxx pixels for title
            if (title_n && picon)                                    // text not empty and icon resource not null
            {                                                        // icon and text will be aligned left
                int icon_w = icon_wh.x + window->padding.left + window->padding.right;
                int title_w = rc_tit.w - icon_w;
                rc_tit.w = icon_w;
                render_icon_align(rc_tit, id_icon, window->color_back, ALIGN_CENTER);
                rc_tit.x = icon_w;
                rc_tit.w = title_w;
                render_text_align(rc_tit, title, window->font_title, window->color_back, window->color_text, window->padding, ALIGN_LEFT_CENTER);
            } else if (title_n) // text not empty and icon resource is null
            {                   // text will be aligned left
                render_text_align(rc_tit, title, window->font_title, window->color_back, window->color_text, window->padding, ALIGN_LEFT_CENTER);
                display->draw_line(point_ui16(rc_tit.x + window->padding.left, rc_tit.y + rc_tit.h),
                    point_ui16(rc_tit.x + rc_tit.w - (window->padding.left + window->padding.right), rc_tit.y + rc_tit.h),
                    COLOR_RED_ALERT);
                red_line_offset = 1;
            } else // text is empty, icon resource not null
            {      // icon will be aligned to center
                render_icon_align(rc_tit, id_icon, window->color_back, ALIGN_CENTER);
            }
        }

        rect_ui16_t rc_txt = window->win.rect;
        rc_txt.h -= (30 + title_h + red_line_offset); // 30pixels for buttons
        rc_txt.y += title_h + red_line_offset;

        render_text_align(rc_txt, window->text, window->font, window->color_back, window->color_text, window->padding, window->alignment | RENDER_FLG_WORDB);
        window->flags |= MSGBOX_MSK_CHG;

        rect_ui16_t rc_btn_bg = window->win.rect;
        rc_btn_bg.y += (rc_btn_bg.h - 40);
        rc_btn_bg.h = 53; //This should be 40 but, there is 13px shortage of gui_defaults.msg_box_sz.h
        display->fill_rect(rc_btn_bg, COLOR_BLACK);

        window_msgbox_draw_buttons(window);
        window->win.flg &= ~WINDOW_FLG_INVALID;
    } else if (window->flags & MSGBOX_MSK_CHG)
        window_msgbox_draw_buttons(window);

    if (window->flags & MSGBOX_GREY_FRAME) { //draw frame
        rect_ui16_t rc = window->win.rect;
        display->draw_line(point_ui16(rc.x, rc.y), point_ui16(239, rc.y), COLOR_GRAY);
        display->draw_line(point_ui16(rc.x, rc.y), point_ui16(rc.x, 320 - 67), COLOR_GRAY);
        display->draw_line(point_ui16(239, rc.y), point_ui16(239, 320 - 67), COLOR_GRAY);
        display->draw_line(point_ui16(rc.x, 320 - 67), point_ui16(239, 320 - 67), COLOR_GRAY);
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
