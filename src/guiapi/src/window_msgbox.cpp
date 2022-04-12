// window_msgbox.cpp
#include "window_msgbox.hpp"
#include "resource.h"
#include "sound.hpp"
#include <algorithm>
#include "ScreenHandler.hpp"
#include "client_response_texts.hpp"
#include "GuiDefaults.hpp"

/*****************************************************************************/
//MsgBoxBase
MsgBoxBase::MsgBoxBase(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels, string_view_utf8 txt, is_multiline multiline)
    : AddSuperWindow<IDialog>(rect)
    , text(this, getTextRect(), multiline, is_closed_on_click_t::no, txt)
    , buttons(this, GuiDefaults::GetButtonRect(rect), resp, labels)
    , result(Response::_none) {
    buttons.SetBtnIndex(def_btn);
    //buttons.SetCapture(); //todo make this work
}

Rect16 MsgBoxBase::getTextRect() {
    return GetRect() - GuiDefaults::GetButtonRect(GetRect()).Height();
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
        break;
    case GUI_event_t::ENC_DN:
        --buttons;
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//MsgBoxTitled
MsgBoxTitled::MsgBoxTitled(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, string_view_utf8 tit, uint16_t title_icon_id_res)
    : AddSuperWindow<MsgBoxIconned>(rect, resp, def_btn, labels, txt, multiline, title_icon_id_res)
    , title(this, Rect16(), is_multiline::no, is_closed_on_click_t::no, tit) {
    // set title params for height extraction
    title.font = getTitleFont();
    title.SetPadding(GuiDefaults::Padding);
    // align icon to the left
    icon.SetRect(getIconRect());
    // set positions of the rest
    title.SetRect(getTitleRect());
    text.SetRect(getTextRect()); // reinit text, icon and title must be initialized
}

Rect16 MsgBoxTitled::getTitleRect() {
    Rect16 title_rect;
    if (!icon.GetRect().IsEmpty()) {
        title_rect = icon.GetRect();                // Y, H is valid
        title_rect += Rect16::Left_t(icon.Width()); // fix X
    } else {
        title_rect = GetRect();                           // X, Y is valid
        title_rect = Rect16::Height_t(getTitleFont()->h); // fix H
    }
    //now just need to calculate W
    title_rect = Rect16::Width_t(Left() + Width() - title_rect.Left());
    return title_rect;
}

Rect16 MsgBoxTitled::getLineRect() {
    return Rect16(GetRect().Left() + title.padding.left, GetRect().Top() + getTitleRect().Height(),
        GetRect().Width() - (title.padding.left + title.padding.right), 1);
}

Rect16 MsgBoxTitled::getTextRect() {
    Rect16 text_rect = GetRect();
    uint16_t x = getLineRect().TopEndPoint().y + GuiDefaults::Padding.top;

    text_rect -= Rect16::Height_t(x - text_rect.Top());
    text_rect -= GuiDefaults::GetButtonRect(GetRect()).Height();

    text_rect = Rect16::Top_t(x);
    return text_rect;
}

Rect16 MsgBoxTitled::getIconRect() {
    return Rect16(GetRect().Left(), GetRect().Top(), GuiDefaults::FooterIconSize.w, std::max((int)GuiDefaults::FooterIconSize.h, title.font->h + title.padding.top + title.padding.bottom));
}

font_t *MsgBoxTitled::getTitleFont() {
    return GuiDefaults::FontBig;
}

void MsgBoxTitled::unconditionalDraw() {
    MsgBoxBase::unconditionalDraw();
    Rect16 line = getLineRect();
    display::DrawLine(line.TopLeft(), line.BottomRight(), COLOR_RED_ALERT);
}

/*****************************************************************************/
//MsgBoxIconned
MsgBoxIconned::MsgBoxIconned(Rect16 rect, const PhaseResponses &resp, size_t def_btn, const PhaseTexts *labels,
    string_view_utf8 txt, is_multiline multiline, uint16_t icon_id_res)
    : AddSuperWindow<MsgBoxBase>(rect, resp, def_btn, labels, txt, multiline)
    , icon(this, icon_id_res, { int16_t(rect.Left()), int16_t(rect.Top()) }, GuiDefaults::Padding) {
    text.SetRect(getTextRect()); // reinit text, icon and title must be initialized
    icon -= Rect16::Width_t(GuiDefaults::Padding.left + GuiDefaults::Padding.right);
    icon += Rect16::Left_t((Width() / 2) - (icon.Width() / 2)); // center icon
}

Rect16 MsgBoxIconned::getTextRect() {
    Rect16 text_rect = GetRect();
    text_rect -= icon.Height();
    text_rect -= GuiDefaults::GetButtonRect(GetRect()).Height();

    text_rect += Rect16::Top_t(icon.Height());
    return text_rect;
}

Rect16 MsgBoxIconned::getIconRect() {
    return Rect16(GuiDefaults::MsgBoxLayoutRect.Left(), GuiDefaults::MsgBoxLayoutRect.Top(), 0, 0);
}

/*****************************************************************************/
//MsgBoxBase variadic template methods
//to be used as blocking functions
template <class T, typename... Args>
Response MsgBox_Custom(Rect16 rect, const PhaseResponses &resp, size_t def_btn, string_view_utf8 txt, is_multiline multiline, Args... args) {
    const PhaseTexts labels = { BtnResponse::GetText(resp[0]), BtnResponse::GetText(resp[1]), BtnResponse::GetText(resp[2]), BtnResponse::GetText(resp[3]) };
    //static_assert(labels.size() == 4, "Incorrect array size, modify number of elements");
    T msgbox(rect, resp, def_btn, &labels, txt, multiline, args...);
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
