#include "DialogRadioButton.hpp"
#include <algorithm> //find
#include "sound.hpp"
#include "../../lang/i18n.h"
#include "resource.h" //IDR_FNT_BIG
#include "gui.hpp"

/*****************************************************************************/
//static variables and methods
static const PhaseResponses no_responses = { Response::_none, Response::_none, Response::_none, Response::_none }; //used in constructor

size_t RadioButton::cnt_labels(const PhaseTexts *labels) {
    if (!labels)
        return 0;
    return (std::find_if(labels->begin(), labels->end(), [](const char *s) { return s[0] == '\0'; })) - labels->begin();
}

size_t RadioButton::cnt_responses(const PhaseResponses *resp) {
    if (!resp)
        return 0;
    return (std::find(resp->begin(), resp->end(), Response::_none)) - resp->begin();
}

size_t RadioButton::cnt_buttons(const PhaseTexts *labels, const PhaseResponses *resp) {
    size_t lbls = std::min(cnt_labels(labels), max_buttons);
    size_t cmds = std::min(cnt_responses(resp), max_buttons);
    return std::max(lbls, cmds);
}
/*****************************************************************************/
//nonstatic variables and methods
RadioButton::RadioButton(window_t *parent, Rect16 rect, const PhaseResponses *resp, const PhaseTexts *labels)
    : AddSuperWindow<window_t>(parent, rect)
    , pfont(resource_font(IDR_FNT_BIG))
    , responses(resp)
    , texts(labels) {
    SetBackColor(COLOR_ORANGE);
    SetBtnCount(cnt_buttons(labels, resp));
    SetBtnIndex(0);
    Enable();
}

//no overflow
RadioButton &RadioButton::operator++() {
    int8_t index = GetBtnIndex();
    if ((index + 1) < GetBtnCount()) {
        SetBtnIndex(index + 1);
        Invalidate();
        Sound_Play(eSOUND_TYPE::EncoderMove);
    } else {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    }
    return *this;
}

//no underflow
RadioButton &RadioButton::operator--() {
    uint8_t index = GetBtnIndex();
    if (index > 0) {
        SetBtnIndex(index - 1);
        Invalidate();
        Sound_Play(eSOUND_TYPE::EncoderMove);
    } else {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    }
    return *this;
}

void RadioButton::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (!GetParent())
        return;

    switch (event) {
    case GUI_event_t::ENC_UP:
        ++(*this);
        return;
    case GUI_event_t::ENC_DN:
        --(*this);
        return;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

void RadioButton::unconditionalDraw() {
    const size_t cnt = GetBtnCount();
    switch (cnt) {
    case 0:
        draw_0_btn(); //cannot use draw_n_btns, would div by 0
        break;
    case 1:
        draw_1_btn(); //could use draw_n_btns, but this is much faster
        break;
    default:
        draw_n_btns(cnt);
        break;
    }
}

Response RadioButton::Click() const {
    if (!responses)
        return Response::_none;
    return (*responses)[GetBtnIndex()];
}

void RadioButton::draw_0_btn() {
    super::unconditionalDraw();
}

void RadioButton::draw_1_btn() {
    if (texts)
        button_draw(GetRect(), GetBackColor(), _((*texts)[0]), pfont, IsEnabled());
}

void RadioButton::draw_n_btns(const size_t btn_count) {
    if (!texts)
        return;
    const uint32_t MAX_TEXT_BUFFER = 128;
    static_assert(sizeof(btn_count) <= GuiDefaults::MAX_DIALOG_BUTTON_COUNT, "Too many RadioButtons to draw.");

    /// fix size of dialog buttons - MAX_DIALOG_BUTTON_COUNT
    Rect16 splits[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];
    Rect16 spaces[GuiDefaults::MAX_DIALOG_BUTTON_COUNT - 1];
    uint8_t text_width[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];

    for (size_t index = 0; index < btn_count; index++) {
        string_view_utf8 txt = _((*texts)[index]);
        text_width[index] = pfont->w * static_cast<uint8_t>(txt.computeNumUtf8CharsAndRewind());
    }
    GetRect().HorizontalSplit(splits, spaces, btn_count, GuiDefaults::ButtonSpacing, text_width);

    for (size_t i = 0; i < btn_count; ++i) {
        string_view_utf8 drawn = _((*texts)[i]);
        char buffer[MAX_TEXT_BUFFER] = { 0 };
        if (text_width[i] > splits[i].Width()) {
            uint32_t max_btn_label_text = splits[i].Width() / pfont->w;
            size_t length = std::min(max_btn_label_text, MAX_TEXT_BUFFER - 1);
            length = drawn.copyToRAM(buffer, length);
            buffer[length] = 0;
            drawn = string_view_utf8::MakeRAM((const uint8_t *)buffer);
        }
        button_draw(splits[i], GetBackColor(), drawn, pfont, GetBtnIndex() == i && IsEnabled());
    }
    color_t spaces_clr = (GetBackColor() == COLOR_ORANGE) ? COLOR_BLACK : COLOR_ORANGE;
    for (size_t i = 0; i < btn_count - 1; ++i) {
        display::FillRect(spaces[i], spaces_clr);
    }
}

void RadioButton::button_draw(Rect16 rc_btn, color_t back_color, string_view_utf8 text, const font_t *pf, bool is_selected) {
    color_t button_cl = is_selected ? back_color : COLOR_GRAY;
    color_t text_cl = is_selected ? COLOR_BLACK : COLOR_WHITE;
    render_text_align(rc_btn, text, pf, button_cl, text_cl, { 0, 0, 0, 0 }, Align_t::Center());
}

bool RadioButton::IsEnabled() const {
    return responses ? (*responses)[0] != Response::_none : false; //faster than cnt_responses(responses)!=0
}

void RadioButton::Change(const PhaseResponses *responses, const PhaseTexts *texts) {
    this->responses = responses;
    this->texts = texts;
    SetBtnCount(cnt_buttons(texts, responses));
    SetBtnIndex(0);
    Invalidate();
}
