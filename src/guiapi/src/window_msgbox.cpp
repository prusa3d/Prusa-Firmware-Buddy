// window_msgbox.cpp
#include "window_msgbox.hpp"
#include "guitypes.h"
#include "resource.h"
#include "button_draw.h"
#include "sound.hpp"
#include <algorithm>
#include "ScreenHandler.hpp"
#include "dialog_response.hpp"

//title for each icon type (empty text for 0)
const char *window_msgbox_title_text[] = {
    "",                // MSGBOX_ICO_CUSTOM     0x0000
    N_("Error"),       // MSGBOX_ICO_ERROR      0x0010
    N_("Question"),    // MSGBOX_ICO_QUESTION   0x0020
    N_("Warning"),     // MSGBOX_ICO_WARNING    0x0030
    N_("Information"), // MSGBOX_ICO_INFO       0x0040
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
    "",              //                      0
    "",              //                      1
    N_("CANCEL"),    // MSGBOX_RES_CANCEL    2
    N_("ABORT"),     // MSGBOX_RES_ABORT     3
    N_("RETRY"),     // MSGBOX_RES_RETRY     4
    N_("IGNORE"),    // MSGBOX_RES_IGNORE    5
    N_("YES"),       // MSGBOX_RES_YES       6
    N_("NO"),        // MSGBOX_RES_NO        7
    N_("OK"),        // MSGBOX_RES_OK        8
    "",              //                      9
    N_("TRY AGAIN"), // MSGBOX_RES_TRYAGAIN 10
    N_("CONTINUE"),  // MSGBOX_RES_CONTINUE 11
    "CUSTOM0",       // MSGBOX_RES_CUSTOM0  12 intentionally not translated
    "CUSTOM1",       // MSGBOX_RES_CUSTOM1  13
    "CUSTOM2",       // MSGBOX_RES_CUSTOM2  14
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
void window_msgbox_t::drawButtons() {
    rect_ui16_t rc_btn = rect;
    rc_btn.y += (rc_btn.h - gui_defaults.btn_h - gui_defaults.frame_width);
    rc_btn.h = gui_defaults.btn_h;

    const int btn = ((flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    if (btn > MSGBOX_BTN_MAX)
        return;                                                   //invalid config - exit
    const int count = window_msgbox_button_count[btn];            // get number of buttons from table
    const uint8_t *button_ids = window_msgbox_buttons[btn];       // get pointer to 3 element button array
    const int idx = ((flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX); // selected button index
    const int chg = ((flags & MSGBOX_MSK_CHG) >> MSGBOX_SHI_CHG); // change mask
    if (chg == 7)
        display::FillRect(rc_btn, color_back);                     //clear background if all buttons changed
    const int spacing2 = gui_defaults.btn_spacing;                 // button spacing
    const int btn_w = (rc_btn.w - (count * 2 * spacing2)) / count; // avg width of a button
    const font_t *pf = font_title;
    float chars = 0; // average number of chars in a button

    for (int i = 0; i < count; i++) {
        if (buttons[i] == NULL) // set default button in case of missing one
            buttons[i] = window_msgbox_button_text[button_ids[i]];
        string_view_utf8 btni = _(buttons[i]); // @@TODO optimize - probably can cache the translated buttons for the cycle below
        chars += btni.computeNumUtf8CharsAndRewind();
    }

    chars /= count;
    rc_btn.x += spacing2;

    for (int i = 0; i < count; i++) {
        string_view_utf8 btni = _(buttons[i]);
        rc_btn.w = btn_w + pf->w * (btni.computeNumUtf8CharsAndRewind() - chars);
        if (chg & (1 << i)) {
            button_draw(rc_btn, btni, pf, i == idx);
        }
        rc_btn.x += rc_btn.w + 2 * spacing2; // next button is 2x spacing to the right
    }

    flags &= ~MSGBOX_MSK_CHG;
}

/// Actions after knob has turned
void window_msgbox_t::step(int step) {
    const int btn = ((flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    const int count = window_msgbox_button_count[btn];            // get number of buttons from table
    int idx = ((flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX);       // selected button index
    int chg = (1 << idx);                                         // change mask - old button
    idx += step;                                                  // increment index
    if (idx < 0) {
        idx = 0; // check min
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    if (idx >= count) {
        idx = count - 1; // check max
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    chg |= (1 << idx);                               // change mask - new button
    flags = (flags & ~MSGBOX_MSK_IDX) |              // clear index bits
        ((idx << MSGBOX_SHI_IDX) & MSGBOX_MSK_IDX) | // set new index bits
        ((chg << MSGBOX_SHI_CHG) & MSGBOX_MSK_CHG);  // set change flags
    gui_invalidate();
}

/// Actions after knob was pushed
void window_msgbox_t::click() {
    const int btn = ((flags & MSGBOX_MSK_BTN) >> MSGBOX_SHI_BTN); // button config
    const int idx = ((flags & MSGBOX_MSK_IDX) >> MSGBOX_SHI_IDX); // selected button index
    res = window_msgbox_buttons[btn][idx];
    Sound_Stop();
    Screens::Access()->Close(); //will set close flag
}

window_msgbox_t::window_msgbox_t(rect_ui16_t rect)
    : IDialog(rect_empty_ui16(rect) ? rect_ui16(0, 0, display::GetW(), display::GetH()) : rect)
    , color_text(gui_defaults.color_text)
    , font(gui_defaults.font)
    , font_title(gui_defaults.font_big)
    , padding(gui_defaults.padding)
    , alignment(ALIGN_CENTER)
    , title(string_view_utf8::MakeNULLSTR())
    , id_icon(0)
    , text(string_view_utf8::MakeNULLSTR())
    , flags(MSGBOX_BTN_OK | MSGBOX_ICO_INFO)
    , res(0) {
    Enable();
}

/// Draws parts of message box that require redraw
void window_msgbox_t::unconditionalDraw() {
    display::FillRect(rect, COLOR_BLACK); // clear window

    uint8_t red_line_offset = 0;
    const int ico = ((flags & MSGBOX_MSK_ICO) >> MSGBOX_SHI_ICO);
    // get title from window member or set default (info, warning, error...)
    string_view_utf8 title = (!title.isNULLSTR()) ? title : _(window_msgbox_title_text[ico]);
    const size_t title_n = title.computeNumUtf8CharsAndRewind(); // number of chars in title
    // title height in pixels; if not empty, use font height
    int title_h = (!title_n) ? 0 : font_title->h;

    // get icon id from window member; for error, warning, info and question -> disable icon
    if (ico < 1 && id_icon == 0)
        id_icon = window_msgbox_id_icon[ico];
    size_ui16_t icon_dim = size_ui16(0, 0);
    const uint8_t *p_icon = 0;                             // icon resource pointer
    if (id_icon && (p_icon = resource_ptr(id_icon))) {     // id_icon is set and resource pointer is not null
        icon_dim = icon_size(p_icon);                      // get icon dimensions
        title_h = std::max(uint16_t(title_h), icon_dim.h); // adjust title height
    }

    if (title_h) {                               // render visible text only (title_h > 0)
        title_h += padding.top + padding.bottom; // add padding
        rect_ui16_t rc_tit = rect;
        rc_tit.h = title_h;      // xxx pixels for title
        if (title_n && p_icon) { // text and icon available => all will be aligned left
            const int icon_w = icon_dim.w + padding.left + padding.right;
            rc_tit.w = icon_w;
            render_icon_align(rc_tit, id_icon, color_back, ALIGN_CENTER);
            rc_tit.x = icon_w;
            rc_tit.w = rect.w - icon_w;
            render_text_align(rc_tit, title, font_title, color_back, color_text, padding, ALIGN_LEFT_CENTER);
        } else if (title_n) { // text not empty but no icon => text will be aligned left
            render_text_align(rc_tit, title, font_title, color_back, color_text, padding, ALIGN_LEFT_CENTER);
            display::DrawLine(point_ui16(rc_tit.x + padding.left, rc_tit.y + rc_tit.h),
                point_ui16(rc_tit.x + rc_tit.w - (padding.left + padding.right), rc_tit.y + rc_tit.h),
                COLOR_RED_ALERT);
            red_line_offset = 1;
        } else { // no text but icon available => icon will be aligned to center
            render_icon_align(rc_tit, id_icon, color_back, ALIGN_CENTER);
        }
    }

    const rect_ui16_t rc_txt = { rect.x,
        uint16_t(rect.y + title_h + red_line_offset), // put text bellow title and red line
        rect.w,
        uint16_t(rect.h - (title_h + red_line_offset + gui_defaults.btn_h)) };
    render_text_align(rc_txt, text, font, color_back, color_text, padding, alignment | RENDER_FLG_WORDB);

    flags |= MSGBOX_MSK_CHG;
    drawButtons();

    if (flags & MSGBOX_GREY_FRAME) {                            /// draw frame
        const uint16_t w = (display::GetW() - 1) - rect.x + 1;  /// last - first + 1
        const uint16_t h = (display::GetH() - 67) - rect.y + 1; /// last - first + 1
        display::DrawRect(rect_ui16(rect.x, rect.y, w, h), COLOR_GRAY);
    }
}

void window_msgbox_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
        click();
        break;
    case WINDOW_EVENT_ENC_DN:
        step(-1);
        break;
    case WINDOW_EVENT_ENC_UP:
        step(+1);
        break;
    default:
        window_frame_t::windowEvent(sender, event, param);
    }
}

/*****************************************************************************/
// clang-format off
const PhaseResponses Responses_Ok               = { Response::Ok,    Response::_none,  Response::_none,  Response::_none };
const PhaseResponses Responses_OkCancel         = { Response::Ok,    Response::Cancel, Response::_none,  Response::_none };
const PhaseResponses Responses_AbortRetryIgnore = { Response::Abort, Response::Retry,  Response::Ignore, Response::_none };
const PhaseResponses Responses_YesNoCancel      = { Response::Yes,   Response::No,     Response::Cancel, Response::_none };
const PhaseResponses Responses_RetryCancel      = { Response::Retry, Response::Cancel, Response::_none,  Response::_none };
// clang-format on
/*****************************************************************************/

/*****************************************************************************/
//MsgBoxBase
MsgBoxBase::MsgBoxBase(rect_ui16_t rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt)
    : IDialog(rect)
    , text(this, getTextRect(), is_closed_on_click_t::no, txt)
    , buttons(this, get_radio_button_size(rect), resp, labels)
    , result(Response::_none) {
    //text.SetAlignment(ALIGN_CENTER);
    //buttons.SetCapture(); //todo make this work
}

rect_ui16_t MsgBoxBase::getTextRect() {
    return { rect.x, rect.y, rect.w, uint16_t(rect.h - get_radio_button_size(rect).h) };
}

Response MsgBoxBase::GetResult() {
    return result;
}

//todo make radio button events behave like normal button
void MsgBoxBase::windowEvent(window_t *sender, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_BTN_DN:
    case WINDOW_EVENT_CLICK:
        result = buttons.Click();
        Screens::Access()->Close();
        break;
    case WINDOW_EVENT_ENC_UP:
        ++buttons;
        gui_invalidate();
        break;
    case WINDOW_EVENT_ENC_DN:
        --buttons;
        gui_invalidate();
        break;
    default:
        IDialog::windowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//MsgBoxTitled
MsgBoxTitled::MsgBoxTitled(rect_ui16_t rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt, string_view_utf8 tit, uint16_t title_icon_id_res)
    : MsgBoxBase(rect, resp, labels, txt)
    , title_icon(this, title_icon_id_res, { rect.x, rect.y }, gui_defaults.padding)
    , title(this, getTitleRect(), is_closed_on_click_t::no, tit) {
    text.rect = getTitledTextRect(); // reinit text, icon and title must be initialized
    title.font = getTitleFont();
}

rect_ui16_t MsgBoxTitled::getTitleRect() {
    rect_ui16_t title_rect;
    if (title_icon.rect.w && title_icon.rect.h) {
        title_rect = title_icon.rect;      // Y, H is valid
        title_rect.x += title_icon.rect.w; // fix X
    } else {
        title_rect = rect;                // X, Y is valid
        title_rect.h = getTitleFont()->h; // fix H
    }
    //now just need to calculate W
    title_rect.w = rect.x + rect.w - title_rect.x;
    return title_rect;
}

rect_ui16_t MsgBoxTitled::getTitledTextRect() {
    rect_ui16_t text_rect = rect;
    text_rect.h -= getTitleRect().h;
    text_rect.h -= 4; // atleast 1px red line and 1px space after red line
    text_rect.h -= get_radio_button_size(rect).h;

    text_rect.y += getTitleRect().h;
    text_rect.y += 4; // atleast 1px red line and 1px space after red line
    return text_rect;
}

font_t *MsgBoxTitled::getTitleFont() {
    return gui_defaults.font_big;
}

void MsgBoxTitled::unconditionalDraw() {
    MsgBoxBase::unconditionalDraw();
    rect_ui16_t rc = getTitledTextRect();

    display::DrawLine(point_ui16(rc.x + title.padding.left, rc.y - 1),
        point_ui16(rc.w - 2 * (title.padding.left + title.padding.right), rc.y - 1),
        COLOR_RED_ALERT);
}

/*****************************************************************************/
//MsgBoxBase variadic template methods
//to be used as blocking functions
template <class T, typename... Args>
Response Call_Custom(rect_ui16_t rect, const PhaseResponses &resp, string_view_utf8 txt, Args... args) {
    const PhaseTexts labels = { BtnTexts::Get(resp[0]), BtnTexts::Get(resp[1]), BtnTexts::Get(resp[2]), BtnTexts::Get(resp[3]) };
    //static_assert(labels.size() == 4, "Incorrect array size, modify number of elements");
    T msgbox(rect, &resp, &labels, txt, args...);
    msgbox.MakeBlocking();
    return msgbox.GetResult();
}

template <class T, typename... Args>
Response Call_BtnOk(string_view_utf8 txt, Args... args) {
    return Call_Custom<T>(gui_defaults.scr_body_sz, Responses_Ok, txt, args...);
}

template <class T, typename... Args>
Response Call_BtnOkCancel(string_view_utf8 txt, Args... args) {
    return Call_Custom<T>(gui_defaults.scr_body_sz, Responses_OkCancel, txt, args...);
}

template <class T, typename... Args>
Response Call_BtnAbortRetryIgnore(string_view_utf8 txt, Args... args) {
    return Call_Custom<T>(gui_defaults.scr_body_sz, Responses_AbortRetryIgnore, txt, args...);
}

template <class T, typename... Args>
Response Call_BtnYesNoCancel(string_view_utf8 txt, Args... args) {
    return Call_Custom<T>(gui_defaults.scr_body_sz, Responses_YesNoCancel, txt, args...);
}

template <class T, typename... Args>
Response Call_BtnRetryCancel(string_view_utf8 txt, Args... args) {
    return Call_Custom<T>(gui_defaults.scr_body_sz, Responses_RetryCancel, txt, args...);
}

Response MsgBox(const PhaseResponses &resp, string_view_utf8 txt) {
    return Call_Custom<MsgBoxBase>(gui_defaults.scr_body_sz, resp, txt);
}

Response MsgBoxError(const PhaseResponses &resp, string_view_utf8 txt) {
    constexpr static const char *label = N_("ERROR");
    static const string_view_utf8 label_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(label));
    return Call_Custom<MsgBoxTitled>(gui_defaults.scr_body_sz, resp, txt, label_view, IDR_PNG_header_icon_error);
}
