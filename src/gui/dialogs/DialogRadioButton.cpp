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
    size_t lbls = cnt_labels(labels);
    size_t cmds = cnt_responses(resp);
    return std::max(lbls, cmds);
}
/*****************************************************************************/
//nonstatic variables and methods
RadioButton::RadioButton(window_t *parent, Rect16 rect, const PhaseResponses *resp, const PhaseTexts *labels)
    : window_t(parent, rect)
    , pfont(resource_font(IDR_FNT_BIG))
    , responses(resp)
    , texts(labels)
    , btn_count(cnt_buttons(labels, resp))
    , selected_index(0) {
    Enable();
}

//no overflow
RadioButton &RadioButton::operator++() {
    if ((selected_index + 1) < btn_count) {
        ++selected_index; //btn_count can be 0
        Invalidate();
    } else {
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    return *this;
}

//no underflow
RadioButton &RadioButton::operator--() {
    if (selected_index > 0) {
        --selected_index;
        Invalidate();
    } else {
        Sound_Play(eSOUND_TYPE_BlindAlert);
    }
    return *this;
}

void RadioButton::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (!GetParent())
        return;

    switch (event) {
    case WINDOW_EVENT_ENC_UP:
        ++(*this);
        return;
    case WINDOW_EVENT_ENC_DN:
        --(*this);
        return;
    default:
        window_t::windowEvent(sender, event, param);
    }
}

void RadioButton::unconditionalDraw() {
    switch (btn_count) {
    case 0:
        draw_0_btn(); //cannot use draw_n_btns, would div by 0
        break;
    case 1:
        draw_1_btn(); //could use draw_n_btns, but this is much faster
        break;
    default:
        draw_n_btns(btn_count);
        break;
    }
}

Response RadioButton::Click() const {
    if (!responses)
        return Response::_none;
    return (*responses)[selected_index];
}

void RadioButton::draw_0_btn() const {
    display::FillRect(rect, color_back);
}

void RadioButton::draw_1_btn() const {
    if (texts)
        button_draw(rect, _((*texts)[0]), pfont, IsEnabled());
}

void RadioButton::draw_n_btns(size_t btn_count) const {
    if (!texts)
        return;

    Rect16 splits[4]; //fix size, dont want to use template
    Rect16 spaces[3];
    rect.VerticalSplit(splits, spaces, btn_count, GuiDefaults::ButtonSpacing);

    for (size_t i = 0; i < btn_count; ++i) {
        button_draw(splits[i], _((*texts)[i]), pfont, selected_index == i && IsEnabled());
    }
    for (size_t i = 0; i < btn_count - 1; ++i) {
        display::FillRect(spaces[i], color_back);
    }
}

void RadioButton::button_draw(Rect16 rc_btn, string_view_utf8 text, const font_t *pf, bool is_selected) {
    color_t back_cl = is_selected ? COLOR_ORANGE : COLOR_GRAY;
    color_t text_cl = is_selected ? COLOR_BLACK : COLOR_WHITE;
    render_text_align(rc_btn, text, pf, back_cl, text_cl, { 0, 0, 0, 0 }, ALIGN_CENTER);
}

bool RadioButton::IsEnabled() const {
    return responses ? (*responses)[0] != Response::_none : false; //faster than cnt_responses(responses)!=0
}

void RadioButton::Change(const PhaseResponses *responses, const PhaseTexts *texts) {
    this->responses = responses;
    this->texts = texts;
    btn_count = cnt_buttons(texts, responses);
    Invalidate();
}
