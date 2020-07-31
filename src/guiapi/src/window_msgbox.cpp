// window_msgbox.cpp
#include "window_msgbox.hpp"
#include "resource.h"
#include "sound.hpp"
#include <algorithm>
#include "ScreenHandler.hpp"
#include "dialog_response.hpp"
#include "GuiDefaults.hpp"

/*****************************************************************************/
// clang-format off
const PhaseResponses Responses_NONE             = { Response::_none, Response::_none,  Response::_none,  Response::_none };
const PhaseResponses Responses_Ok               = { Response::Ok,    Response::_none,  Response::_none,  Response::_none };
const PhaseResponses Responses_OkCancel         = { Response::Ok,    Response::Cancel, Response::_none,  Response::_none };
const PhaseResponses Responses_AbortRetryIgnore = { Response::Abort, Response::Retry,  Response::Ignore, Response::_none };
const PhaseResponses Responses_YesNo            = { Response::Yes,   Response::No,     Response::_none,  Response::_none };
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
    , title_icon(this, title_icon_id_res, { rect.x, rect.y }, GuiDefaults::PaddingGet())
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
    return GuiDefaults::FontBig;
}

void MsgBoxTitled::unconditionalDraw() {
    MsgBoxBase::unconditionalDraw();
    rect_ui16_t rc = getTitledTextRect();

    display::DrawLine(point_ui16(rc.x + title.padding.left, rc.y - 1),
        point_ui16(rc.w - 2 * (title.padding.left + title.padding.right), rc.y - 1),
        COLOR_RED_ALERT);
}

/*****************************************************************************/
//MsgBoxIconned
MsgBoxIconned::MsgBoxIconned(rect_ui16_t rect, const PhaseResponses *resp, const PhaseTexts *labels, string_view_utf8 txt, uint16_t icon_id_res)
    : MsgBoxBase(rect, resp, labels, txt)
    , icon(this, icon_id_res, { rect.x, rect.y }, GuiDefaults::Padding) {
    text.rect = getIconnedTextRect(); // reinit text, icon and title must be initialized
    icon.rect.w = rect.w - GuiDefaults::Padding.left - GuiDefaults::Padding.right;
}

rect_ui16_t MsgBoxIconned::getIconnedTextRect() {
    rect_ui16_t text_rect = rect;
    text_rect.h -= icon.rect.h;
    text_rect.h -= get_radio_button_size(rect).h;

    text_rect.y += icon.rect.h;
    return text_rect;
}

/*****************************************************************************/
//MsgBoxBase variadic template methods
//to be used as blocking functions
template <class T, typename... Args>
Response MsgBox_Custom(rect_ui16_t rect, const PhaseResponses &resp, string_view_utf8 txt, Args... args) {
    const PhaseTexts labels = { BtnTexts::Get(resp[0]), BtnTexts::Get(resp[1]), BtnTexts::Get(resp[2]), BtnTexts::Get(resp[3]) };
    //static_assert(labels.size() == 4, "Incorrect array size, modify number of elements");
    T msgbox(rect, &resp, &labels, txt, args...);
    msgbox.MakeBlocking();
    return msgbox.GetResult();
}

Response MsgBox(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect) {
    return MsgBox_Custom<MsgBoxBase>(rect, resp, txt);
}

Response MsgBoxError(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect) {
    constexpr static const char *label = N_("Error");
    static const string_view_utf8 label_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(label));
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, txt, label_view, IDR_PNG_header_icon_error);
}

Response MsgBoxQuestion(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect) {
    constexpr static const char *label = N_("Question");
    static const string_view_utf8 label_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(label));
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, txt, label_view, IDR_PNG_header_icon_question);
}

Response MsgBoxWarning(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect) {
    constexpr static const char *label = N_("Warning");
    static const string_view_utf8 label_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(label));
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, txt, label_view, IDR_PNG_header_icon_warning);
}

Response MsgBoxTitle(string_view_utf8 title, string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect, uint16_t icon_id) {
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, txt, title, icon_id);
}

Response MsgBoxInfo(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect) {
    constexpr static const char *label = N_("Information");
    static const string_view_utf8 label_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(label));
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, txt, label_view, IDR_PNG_header_icon_info);
}

Response MsgBoxIcon(string_view_utf8 txt, uint16_t icon_id, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect) {
    return MsgBox_Custom<MsgBoxIconned>(rect, resp, txt, icon_id);
}

Response MsgBoxPepa(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, rect_ui16_t rect) {
    return MsgBoxIcon(txt, IDR_PNG_icon_pepa, resp, def_btn, rect);
}
