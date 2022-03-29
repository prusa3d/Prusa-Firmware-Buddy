#include "DialogRadioButton.hpp"
#include <algorithm> //find
#include "sound.hpp"
#include "../../lang/i18n.h"
#include "resource.h" //IDR_FNT_BIG
#include "gui.hpp"

static constexpr uint8_t button_delim_size = 31;
static constexpr uint8_t button_base_size = 70;
static constexpr uint8_t label_delim_size = 11;
static constexpr uint8_t label_base_size = button_base_size + 20;

/*****************************************************************************/
//static variables and methods
static const RadioButton::Responses_t no_responses = { Response::_none, Response::_none, Response::_none, Response::_none }; //used in constructor

size_t RadioButton::cnt_labels(const PhaseTexts *labels) {
    if (!labels)
        return 0;
    return (std::find_if(labels->begin(), labels->end(), [](const char *s) { return s[0] == '\0'; })) - labels->begin();
}

size_t RadioButton::cnt_responses(Responses_t resp) {
    return (std::find(resp.begin(), resp.end(), Response::_none)) - resp.begin();
}

size_t RadioButton::cnt_buttons(const PhaseTexts *labels, Responses_t resp) {
    size_t lbls = std::min(cnt_labels(labels), max_buttons);
    size_t cmds = std::min(cnt_responses(resp), max_buttons);
    return std::max(lbls, cmds);
}

/*****************************************************************************/
//nonstatic variables and methods

RadioButton::RadioButton(window_t *parent, Rect16 rect)
    : RadioButton(parent, rect, Responses_t({ { Response::_none, Response::_none, Response::_none, Response::_none } })) {
}

RadioButton::RadioButton(window_t *parent, Rect16 rect, const PhaseResponses &resp, const PhaseTexts *labels)
    : RadioButton(parent, rect, generateResponses(resp), labels) {
}

RadioButton::RadioButton(window_t *parent, Rect16 rect, Responses_t resp, const PhaseTexts *labels)
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
    if (isIndexValid(index + 1)) {
        SetBtnIndex(index + 1);
        invalidateWhatIsNeeded();
        Sound_Play(eSOUND_TYPE::EncoderMove);
    } else {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    }
    return *this;
}

//no underflow
RadioButton &RadioButton::operator--() {
    uint8_t index = GetBtnIndex();
    if (index > 0 && (isIndexValid(index - 1))) {
        SetBtnIndex(index - 1);
        invalidateWhatIsNeeded();
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
    if (!HasIcon()) {
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
    } else {
        window_t *const prev_focus = focused_ptr; // store focus
        //draw background
        if (!HasValidBackground()) {
            draw_0_btn();
            ValidateBackground();
        }
        //draw foreground
        for (size_t i = 0; i < max_icons; ++i) {
            Rect16 rcIcon = getIconRect(i);
            if (isIndexValid(i)) {
                window_frame_t base(nullptr, rcIcon); // window_icon_button_t needs parrent to draw properly
                base.SetBackColor(GetBackColor());
                window_icon_button_t icon(&base, rcIcon, BtnResponse::GetIconId(responseFromIndex(i)), []() {});
                window_text_t label(nullptr, getLabelRect(i), is_multiline::no, is_closed_on_click_t::no, _(BtnResponse::GetText(responseFromIndex(i))));

                label.SetBackColor(GetBackColor());
                label.SetAlignment(Align_t::Center());
                label.SetFont(resource_font(IDR_FNT_SMALL));

                if (i == GetBtnIndex()) {
                    focused_ptr = &icon;
                }
                icon.Draw();
                label.Draw();
                if (i == GetBtnIndex()) {
                    focused_ptr = nullptr;
                }
            } else {
                display::DrawRect(rcIcon, GetBackColor());
            }
        }
        focused_ptr = prev_focus; //return focus
    }
}

Response RadioButton::Click() const {
    return responseFromIndex(GetBtnIndex());
}

Response RadioButton::responseFromIndex(size_t index) const {
    if (index >= maxSize())
        return Response::_none;
    return (responses)[index];
}

void RadioButton::draw_0_btn() {
    if (GetParent()) {
        display::FillRect(GetRect(), GetParent()->GetBackColor());
    }
}

// called internally, responses must exist
void RadioButton::draw_1_btn() {
    const char *txt_to_print = texts ? (*texts)[0] : BtnResponse::GetText(responseFromIndex(0));
    button_draw(GetRect(), GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), _(txt_to_print), pfont, IsEnabled());
}

// called internally, responses must exist
void RadioButton::draw_n_btns(const size_t btn_count) {
    PhaseTexts txts_to_print;
    if (texts) {
        txts_to_print = *texts;
    } else {
        for (size_t i = 0; i < max_buttons; ++i) {
            txts_to_print[i] = BtnResponse::GetText(responseFromIndex(i));
        }
    }

    const uint32_t MAX_TEXT_BUFFER = 128;
    static_assert(sizeof(btn_count) <= GuiDefaults::MAX_DIALOG_BUTTON_COUNT, "Too many RadioButtons to draw.");

    /// fix size of dialog buttons - MAX_DIALOG_BUTTON_COUNT
    Rect16 splits[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];
    Rect16 spaces[GuiDefaults::MAX_DIALOG_BUTTON_COUNT - 1];
    uint8_t text_width[GuiDefaults::MAX_DIALOG_BUTTON_COUNT];

    for (size_t index = 0; index < btn_count; index++) {
        string_view_utf8 txt = _(txts_to_print[index]);
        text_width[index] = pfont->w * static_cast<uint8_t>(txt.computeNumUtf8CharsAndRewind());
    }
    GetRect().HorizontalSplit(splits, spaces, btn_count, GuiDefaults::ButtonSpacing, text_width);

    for (size_t i = 0; i < btn_count; ++i) {
        string_view_utf8 drawn = _(txts_to_print[i]);
        char buffer[MAX_TEXT_BUFFER] = { 0 };
        if (text_width[i] > splits[i].Width()) {
            uint32_t max_btn_label_text = splits[i].Width() / pfont->w;
            size_t length = std::min(max_btn_label_text, MAX_TEXT_BUFFER - 1);
            length = drawn.copyToRAM(buffer, length);
            buffer[length] = 0;
            drawn = string_view_utf8::MakeRAM((const uint8_t *)buffer);
        }
        button_draw(splits[i], GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), drawn, pfont, GetBtnIndex() == i && IsEnabled());
    }
    color_t spaces_clr = (GetBackColor() == COLOR_ORANGE) ? COLOR_BLACK : COLOR_ORANGE;
    for (size_t i = 0; i < btn_count - 1; ++i) {
        display::FillRect(spaces[i], spaces_clr);
    }
}

void RadioButton::button_draw(Rect16 rc_btn, color_t back_color, color_t parent_color, string_view_utf8 text, const font_t *pf, bool is_selected) {
    color_t button_cl = is_selected ? back_color : COLOR_GRAY;
    color_t text_cl = is_selected ? COLOR_BLACK : COLOR_WHITE;
    if (GuiDefaults::RadioButtonCornerRadius) {
        display::DrawRect(rc_btn, parent_color);
        rc_btn += Rect16::Left_t(GuiDefaults::RadioButtonCornerRadius);
        rc_btn -= Rect16::Width_t(2 * GuiDefaults::RadioButtonCornerRadius);
    }
    render_text_align(rc_btn, text, pf, button_cl, text_cl, { 0, 0, 0, 0 }, Align_t::Center());
}

bool RadioButton::IsEnabled() const {
    return responseFromIndex(0) != Response::_none; //faster than cnt_responses(responses)!=0
}

void RadioButton::Change(const PhaseResponses &resp, const PhaseTexts *txts) {
    Change(generateResponses(resp), txts);
}

void RadioButton::Change(Responses_t resp, const PhaseTexts *txts) {
    if ((responses == resp) && (txts == texts))
        return;
    responses = resp;
    texts = txts;
    SetBtnCount(HasIcon() ? max_icons : cnt_buttons(texts, responses));

    //in iconned layout index will stay
    if (!HasIcon()) {
        SetBtnIndex(0);
    }

    validateBtnIndex();

    invalidateWhatIsNeeded();
}

/**
 * @brief more advanced validation meant pimary for icons
 * iconned layout support to first or second response to be _none
 * if it is focused, focus must shift
 * if no valid response is found, index shall be 0
 */
void RadioButton::validateBtnIndex() {
    if (isIndexValid(GetBtnIndex()))
        return; //index valid

    //default index for not iconned is 0
    if (!HasIcon()) {
        SetBtnIndex(0);
        return;
    }

    if (HasIcon()) {
        SetBtnIndex(1); // default index for iconned is 1
        if (isIndexValid(GetBtnIndex()))
            return; //index 1 is valid

        for (size_t i = 0; i < max_icons; ++i) {
            if (responseFromIndex(i) != Response::_none) {
                SetBtnIndex(i);
                return;
            }
        }
    }
}

bool RadioButton::isIndexValid(size_t index) {
    if (HasIcon()) {
        return (responseFromIndex(index) != Response::_none);
    }

    return index < GetBtnCount();
}

/**
 * @brief does not invalidate background if not needed
 * iconned layout is on fixed positions
 * so it is possible to validate background if it was valid before
 */
void RadioButton::invalidateWhatIsNeeded() {
    bool validate_background = false;
    if (HasIcon() && HasValidBackground()) {
        validate_background = true;
    }
    Invalidate();
    if (validate_background) {
        ValidateBackground();
    }
}

Rect16 RadioButton::getIconRect(uint8_t idx) const {
    Rect16 ret = GetRect();
    int offset = int(idx) - 1; // 3 buttons 0 - 2, button 1 is in middle

    ret += Rect16::Left_t(ret.Width() / 2);                                 // middle of rect
    ret -= Rect16::Left_t(button_base_size / 2);                            // button 1 pos
    ret += Rect16::Left_t(offset * (button_base_size + button_delim_size)); // current button pos
    ret = Rect16::Width_t(button_base_size);                                // button width
    ret = Rect16::Height_t(button_base_size);                               // button height
    return ret;
}

Rect16 RadioButton::getLabelRect(uint8_t idx) const {
    Rect16 ret = GetRect();
    int offset = int(idx) - 1; // 3 labels 0 - 2, button 1 is in middle

    ret += Rect16::Top_t(button_base_size);                               // label is under button
    ret += Rect16::Left_t(ret.Width() / 2);                               // middle of rect
    ret -= Rect16::Left_t(label_base_size / 2);                           // labels 1 pos
    ret += Rect16::Left_t(offset * (label_base_size + label_delim_size)); // current labels pos
    ret = Rect16::Width_t(label_base_size);                               // labels width
    ret -= Rect16::Height_t(label_base_size);                             // labels height
    return ret;
}

size_t RadioButton::maxSize() const {
    return HasIcon() ? max_icons : max_buttons;
}

// 4th response for iconned layout is ensured to be _none
RadioButton::Responses_t RadioButton::generateResponses(const PhaseResponses &resp) const {
    Responses_t newResponses;
    newResponses[3] = Response::_none;
    for (size_t i = 0; i < maxSize(); ++i) {
        newResponses[i] = resp[i];
    }
    return newResponses;
};
