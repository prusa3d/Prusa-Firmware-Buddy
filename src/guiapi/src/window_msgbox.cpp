// window_msgbox.cpp
#include "window_msgbox.hpp"
#include "resource.h"
#include "sound.hpp"
#include <algorithm>
#include "ScreenHandler.hpp"
#include "client_response_texts.hpp"
#include "GuiDefaults.hpp"

/*****************************************************************************/
// clang-format off
const PhaseResponses Responses_NONE             = { Response::_none, Response::_none,  Response::_none,  Response::_none };
const PhaseResponses Responses_Next             = { Response::Next,  Response::_none,  Response::_none,  Response::_none };
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
MsgBoxBase::MsgBoxBase(Rect16 rect, const PhaseResponses *resp, size_t def_btn, const PhaseTexts *labels, string_view_utf8 txt, is_multiline multiline)
    : AddSuperWindow<IDialog>(rect)
    , text(this, getTextRect(), multiline, is_closed_on_click_t::no, txt)
    , buttons(this, get_radio_button_rect(rect), resp, labels)
    , result(Response::_none) {
    buttons.SetBtnIndex(def_btn);
    //text.SetAlignment(Align_t::Center());
    //buttons.SetCapture(); //todo make this work
}

Rect16 MsgBoxBase::getTextRect() {
    return GetRect() - get_radio_button_rect(GetRect()).Height();
}

Response MsgBoxBase::GetResult() {
    return result;
}

//todo make radio button events behave like normal button
void MsgBoxBase::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
        result = buttons.Click();
        Screens::Access()->Close();
        break;
    case GUI_event_t::ENC_UP:
        ++buttons;
        gui_invalidate();
        break;
    case GUI_event_t::ENC_DN:
        --buttons;
        gui_invalidate();
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//MsgBoxTitled
MsgBoxTitled::MsgBoxTitled(Rect16 rect, const PhaseResponses *resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, string_view_utf8 tit, uint16_t title_icon_id_res)
    : AddSuperWindow<MsgBoxBase>(rect, resp, def_btn, labels, txt, multiline)
    , title_icon(this, title_icon_id_res, { rect.Left(), rect.Top() }, GuiDefaults::Padding)
    , title(this, getTitleRect(), is_multiline::no, is_closed_on_click_t::no, tit) {
    text.SetRect(getTitledTextRect()); // reinit text, icon and title must be initialized
    title.font = getTitleFont();
    title.SetPadding({ 0, 0, 0, 0 });
}

Rect16 MsgBoxTitled::getTitleRect() {
    Rect16 title_rect;
    if (!title_icon.GetRect().IsEmpty()) {
        title_rect = title_icon.GetRect();                // Y, H is valid
        title_rect += Rect16::Left_t(title_icon.Width()); // fix X
    } else {
        title_rect = GetRect();                           // X, Y is valid
        title_rect = Rect16::Height_t(getTitleFont()->h); // fix H
    }
    //now just need to calculate W
    title_rect = Rect16::Width_t(Left() + Width() - title_rect.Left());
    return title_rect;
}

Rect16 MsgBoxTitled::getTitledTextRect() {
    Rect16 text_rect = GetRect();
    text_rect -= getTitleRect().Height();
    text_rect -= Rect16::Height_t(4); // atleast 1px red line and 1px space after red line
    text_rect -= get_radio_button_rect(GetRect()).Height();

    text_rect += Rect16::Top_t(getTitleRect().Height());
    text_rect += Rect16::Top_t(4); // atleast 1px red line and 1px space after red line
    return text_rect;
}

font_t *MsgBoxTitled::getTitleFont() {
    return GuiDefaults::FontBig;
}

void MsgBoxTitled::unconditionalDraw() {
    MsgBoxBase::unconditionalDraw();
    Rect16 rc = getTitledTextRect();

    display::DrawLine(point_ui16(rc.Left() + title.padding.left, rc.Top() - 1),
        point_ui16(rc.Width() - 2 * (title.padding.left + title.padding.right), rc.Top() - 1),
        COLOR_RED_ALERT);
}

/*****************************************************************************/
//MsgBoxIconned
MsgBoxIconned::MsgBoxIconned(Rect16 rect, const PhaseResponses *resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, uint16_t icon_id_res)
    : AddSuperWindow<MsgBoxBase>(rect, resp, def_btn, labels, txt, multiline)
    , icon(this, icon_id_res, { int16_t(rect.Left()), int16_t(rect.Top()) }, GuiDefaults::Padding) {
    text.SetRect(getIconnedTextRect()); // reinit text, icon and title must be initialized
    icon -= Rect16::Width_t(GuiDefaults::Padding.left + GuiDefaults::Padding.right);
    icon += Rect16::Left_t((Width() / 2) - (icon.Width() / 2)); // center icon
}

Rect16 MsgBoxIconned::getIconnedTextRect() {
    Rect16 text_rect = GetRect();
    text_rect -= icon.Height();
    text_rect -= get_radio_button_rect(GetRect()).Height();

    text_rect += Rect16::Top_t(icon.Height());
    return text_rect;
}

/*****************************************************************************/
//MsgBoxBase variadic template methods
//to be used as blocking functions
template <class T, typename... Args>
Response MsgBox_Custom(Rect16 rect, const PhaseResponses &resp, size_t def_btn, string_view_utf8 txt, is_multiline multiline, Args... args) {
    const PhaseTexts labels = { BtnTexts::Get(resp[0]), BtnTexts::Get(resp[1]), BtnTexts::Get(resp[2]), BtnTexts::Get(resp[3]) };
    //static_assert(labels.size() == 4, "Incorrect array size, modify number of elements");
    T msgbox(rect, &resp, def_btn, &labels, txt, multiline, args...);
    msgbox.MakeBlocking();
    return msgbox.GetResult();
}

Response MsgBox(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    return MsgBox_Custom<MsgBoxBase>(rect, resp, def_btn, txt, multiline);
}

Response MsgBoxError(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    constexpr static const char *label = N_("Error");
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), IDR_PNG_error_16px);
}

Response MsgBoxQuestion(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    constexpr static const char *label = N_("Question");
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), IDR_PNG_question_16px);
}

Response MsgBoxWarning(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    constexpr static const char *label = N_("Warning");
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), IDR_PNG_warning_16px);
}

Response MsgBoxTitle(string_view_utf8 title, string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, uint16_t icon_id, is_multiline multiline) {
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, title, icon_id);
}

Response MsgBoxInfo(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    constexpr static const char *label = N_("Information");
    return MsgBox_Custom<MsgBoxTitled>(rect, resp, def_btn, txt, multiline, _(label), IDR_PNG_info_16px);
}

Response MsgBoxIcon(string_view_utf8 txt, uint16_t icon_id, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    return MsgBox_Custom<MsgBoxIconned>(rect, resp, def_btn, txt, multiline, icon_id);
}

Response MsgBoxPepa(string_view_utf8 txt, const PhaseResponses &resp, size_t def_btn, Rect16 rect, is_multiline multiline) {
    return MsgBoxIcon(txt, IDR_PNG_pepa_64px, resp, def_btn, rect, multiline);
}
