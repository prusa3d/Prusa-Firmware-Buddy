#include "radio_button.hpp"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "../../lang/i18n.h"
#include "resource.h" //IDR_FNT_BIG
#include "gui.hpp"

#include <algorithm> //find

static constexpr uint8_t button_delim_size = 31;
static constexpr uint8_t button_base_size = GuiDefaults::IconButtonSize;
static constexpr uint8_t label_delim_size = 11;
static constexpr uint8_t label_base_size = button_base_size + 20;
static constexpr uint8_t icon_button_font_height = 16;
static constexpr uint8_t icon_label_delim = 5;

/*****************************************************************************/
//static variables and methods
static const IRadioButton::Responses_t no_responses = { Response::_none, Response::_none, Response::_none, Response::_none }; //used in constructor

size_t IRadioButton::cnt_labels(const PhaseTexts *labels) {
    if (!labels)
        return 0;
    return (std::find_if(labels->begin(), labels->end(), [](const char *s) { return s[0] == '\0'; })) - labels->begin();
}

size_t IRadioButton::cnt_responses(Responses_t resp) {
    return (std::find(resp.begin(), resp.end(), Response::_none)) - resp.begin();
}

size_t IRadioButton::cnt_buttons(const PhaseTexts *labels, Responses_t resp) {
    size_t lbls = std::min(cnt_labels(labels), max_buttons);
    size_t cmds = std::min(cnt_responses(resp), max_buttons);
    return std::max(lbls, cmds);
}

/*****************************************************************************/
//nonstatic variables and methods

IRadioButton::IRadioButton(window_t *parent, Rect16 rect, size_t count)
    : AddSuperWindow<window_t>(parent, rect)
    , pfont(resource_font(IDR_FNT_BIG)) {
    SetBackColor(COLOR_ORANGE);
    SetBtnCount(count);
    SetBtnIndex(0);
    Enable();
}

//no overflow
IRadioButton &IRadioButton::operator++() {
    int8_t index = GetBtnIndex();
    if (isIndexValid(index + 1)) {
        SetBtnIndex(index + 1);
        Sound_Play(eSOUND_TYPE::EncoderMove);
    } else {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    }
    return *this;
}

//no underflow
IRadioButton &IRadioButton::operator--() {
    uint8_t index = GetBtnIndex();
    if (index > 0 && (isIndexValid(index - 1))) {
        SetBtnIndex(index - 1);
        Sound_Play(eSOUND_TYPE::EncoderMove);
    } else {
        Sound_Play(eSOUND_TYPE::BlindAlert);
    }
    return *this;
}

void IRadioButton::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (!GetParent())
        return;

    switch (event) {
    case GUI_event_t::CLICK: {
        // send response to parent via GUI_event_t::CHILD_CLICK
        Response response = Click();
        event_conversion_union un;
        un.response = response;
        if (GetParent())
            GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
    } break;
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

void IRadioButton::unconditionalDraw() {
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
        for (size_t i = 0; i < std::min<size_t>(max_icons, GetBtnCount()); ++i) {
            // Iconed buttons support horizontal and vertical alignment
            // If passed rectangle has bigger width than height -> horizontal alignment is used
            // If passed rectangle has bigger height than width -> vertical alignment is used

            Rect16 rcIcon = getIconRect(i);
            if (isIndexValid(i)) {
                window_frame_t base(nullptr, rcIcon); // window_icon_button_t needs parent to draw properly (corners)
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

Response IRadioButton::Click() const {
    return responseFromIndex(GetBtnIndex());
}

void IRadioButton::draw_0_btn() {
    if (GetParent()) {
        display::FillRect(GetRect(), GetParent()->GetBackColor());
    }
}

// called internally, responses must exist
void IRadioButton::draw_1_btn() {
    const char *txt_to_print = getAlternativeTexts() ? (*getAlternativeTexts())[0] : BtnResponse::GetText(responseFromIndex(0));
    button_draw(GetRect(), GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), _(txt_to_print), pfont, IsEnabled());
}

// called internally, responses must exist
void IRadioButton::draw_n_btns(size_t btn_count) {
    const uint32_t MAX_TEXT_BUFFER = 128;
    Layout layout = getNormalBtnRects(btn_count);

    for (size_t i = 0; i < btn_count; ++i) {
        string_view_utf8 drawn = _(layout.txts_to_print[i]);
        char buffer[MAX_TEXT_BUFFER] = { 0 };
        if (layout.text_widths[i] > layout.splits[i].Width()) {
            uint32_t max_btn_label_text = layout.splits[i].Width() / pfont->w;
            size_t length = std::min(max_btn_label_text, MAX_TEXT_BUFFER - 1);
            length = drawn.copyToRAM(buffer, length);
            buffer[length] = 0;
            drawn = string_view_utf8::MakeRAM((const uint8_t *)buffer);
        }
        button_draw(layout.splits[i], GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), drawn, pfont, GetBtnIndex() == i && IsEnabled());
    }
    color_t spaces_clr = (GetBackColor() == COLOR_ORANGE) ? COLOR_BLACK : COLOR_ORANGE;
    for (size_t i = 0; i < btn_count - 1; ++i) {
        display::FillRect(layout.spaces[i], spaces_clr);
    }
}

IRadioButton::Layout IRadioButton::getNormalBtnRects(size_t btn_count) const {
    Layout ret;
    if (getAlternativeTexts()) {
        ret.txts_to_print = *getAlternativeTexts();
    } else {
        for (size_t i = 0; i < max_buttons; ++i) {
            ret.txts_to_print[i] = BtnResponse::GetText(responseFromIndex(i));
        }
    }

    static_assert(sizeof(btn_count) <= GuiDefaults::MAX_DIALOG_BUTTON_COUNT, "Too many IRadioButtons to draw.");

    for (size_t index = 0; index < btn_count; index++) {
        string_view_utf8 txt = _(ret.txts_to_print[index]);
        ret.text_widths[index] = pfont->w * static_cast<uint8_t>(txt.computeNumUtf8CharsAndRewind());
    }
    GetRect().HorizontalSplit(ret.splits, ret.spaces, btn_count, GuiDefaults::ButtonSpacing, ret.text_widths);

    return ret;
}

void IRadioButton::button_draw(Rect16 rc_btn, color_t back_color, color_t parent_color, string_view_utf8 text, const font_t *pf, bool is_selected) {
    color_t button_cl = is_selected ? back_color : COLOR_GRAY;
    color_t text_cl = is_selected ? COLOR_BLACK : COLOR_WHITE;
    if (GuiDefaults::RadioButtonCornerRadius) {
        display::DrawRect(rc_btn, parent_color);
        rc_btn += Rect16::Left_t(GuiDefaults::RadioButtonCornerRadius);
        rc_btn -= Rect16::Width_t(2 * GuiDefaults::RadioButtonCornerRadius);
    }
    render_text_align(rc_btn, text, pf, button_cl, text_cl, { 0, 0, 0, 0 }, Align_t::Center());
}

bool IRadioButton::IsEnabled() const {
    return responseFromIndex(0) != Response::_none; //faster than cnt_responses(responses)!=0
}

/**
 * @brief more advanced validation meant pimary for icons
 * iconned layout support to first or second response to be _none
 * if it is focused, focus must shift
 * if no valid response is found, index shall be 0
 */
void IRadioButton::validateBtnIndex() {
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

bool IRadioButton::isIndexValid(size_t index) {
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
void IRadioButton::invalidateWhatIsNeeded() {
    bool validate_background = false;
    if (HasIcon() && HasValidBackground()) {
        validate_background = true;
    }
    Invalidate();
    if (validate_background) {
        ValidateBackground();
    }
}

void IRadioButton::SetBtnIndex(uint8_t index) {
    uint8_t idx = (index < GetBtnCount()) ? index : 0;
    if (idx != flags.button_index) {
        flags.button_index = idx;
        invalidateWhatIsNeeded();
    }
}

void IRadioButton::SetBtn(Response btn) {
    auto index = IndexFromResponse(btn);
    if (index)
        SetBtnIndex(*index);
}

//TODO just use some kind of layouts
Rect16 IRadioButton::getIconRect(uint8_t idx) const {
    if (GetRect().Width() >= GetRect().Height()) {
        return getHorizontalIconRect(idx); // 3 buttons 0 - 2, button 1 is in middle
    } else {
        return getVerticalIconRect(idx); // 3 buttons 0 - 2, button 1 is in middle
    }
}

Rect16 IRadioButton::getHorizontalIconRect(uint8_t idx) const {
    const int padding = 10;
    Rect16 rect = GetRect();
    rect = Rect16::Width_t(button_base_size);  // button width
    rect = Rect16::Height_t(button_base_size); // button height

    switch (std::min<size_t>(max_icons, GetBtnCount())) {
    case 1:
        rect += Rect16::Left_t(GetRect().Width() / 2); // middle of rect
        rect -= Rect16::Left_t(button_base_size / 2);  // button 1 pos
        break;
    case 2:
        rect = Rect16::Left_t(idx == 0 ? padding : (GetRect().Width() - button_base_size - padding));
        break;
    case 3:
        rect += Rect16::Left_t(GetRect().Width() / 2);                        // middle of rect
        rect -= Rect16::Left_t(button_base_size / 2);                         // button 1 pos
        rect += Rect16::Left_t(idx * (button_base_size + button_delim_size)); // current button pos
        break;
    default:
        rect = Rect16();
        break;
    }
    return rect;
}

Rect16 IRadioButton::getVerticalIconRect(uint8_t idx) const {
    // Vertical alignment of 3 items: first item on the top edge - second item on centered (with text) - third item's label text on the bottom edge
    // Vertical alignment of 2 items: first item on the top edge - second item's label on the bottom edge
    // Vertical alignment of 1 item: centered with label
    // All centered (x-axis-wise)

    Rect16 rect = GetRect();
    rect += Rect16::Left_t((rect.Width() - button_base_size) / 2);
    rect = Rect16::Width_t(button_base_size);  // button width
    rect = Rect16::Height_t(button_base_size); // button height
    switch (std::min<size_t>(max_icons, GetBtnCount())) {
    case 1:
        rect += Rect16::Top_t(GetRect().Height() / 2);                                              // middle of rect
        rect -= Rect16::Top_t((button_base_size + icon_label_delim + icon_button_font_height) / 2); // button 1 pos
        break;
    case 2:
        if (idx != 0) {
            rect = Rect16::Top_t(GetRect().Top() + GetRect().Height() - button_base_size - icon_label_delim - icon_button_font_height);
        }
        break;
    case 3:
        if (idx == 1) {
            rect += Rect16::Top_t(GetRect().Height() / 2);
            rect -= Rect16::Top_t((button_base_size + icon_label_delim + icon_button_font_height) / 2);
        } else if (idx == 2) {
            rect += Rect16::Top_t(GetRect().Height());
            rect -= Rect16::Top_t(button_base_size + icon_label_delim + icon_button_font_height);
        }
        // else: no change
        break;
    default:
        rect = Rect16();
        break;
    }
    return rect;
}

//TODO just use some kind of layouts
Rect16 IRadioButton::getLabelRect(uint8_t idx) const {
    if (GetRect().Width() >= GetRect().Height()) {
        return getHorizontalLabelRect(idx); // 3 buttons 0 - 2, button 1 is in middle
    } else {
        return getVerticalLabelRect(idx); // 3 buttons 0 - 2, button 1 is in middle
    }
}

Rect16 IRadioButton::getHorizontalLabelRect(uint8_t idx) const {
    Rect16 rect = GetRect();
    rect += Rect16::Top_t(button_base_size);    // label is under button
    rect = Rect16::Width_t(label_base_size);    // labels width
    rect -= Rect16::Height_t(button_base_size); // labels height

    switch (std::min<size_t>(max_icons, GetBtnCount())) {
    case 1:
        rect += Rect16::Left_t(GetRect().Width() / 2); // middle of rect
        rect -= Rect16::Left_t(label_base_size / 2);   // labels 1 pos
        break;
    case 2:
        rect = Rect16::Left_t(idx == 0 ? 0 : (GetRect().Width() - label_base_size));
        break;
    case 3:
        rect += Rect16::Left_t(GetRect().Width() / 2);                      // middle of rect
        rect -= Rect16::Left_t(label_base_size / 2);                        // labels 1 pos
        rect += Rect16::Left_t(idx * (label_base_size + label_delim_size)); // current labels pos
        break;
    default:
        rect = Rect16();
        break;
    }
    return rect;
}

Rect16 IRadioButton::getVerticalLabelRect(uint8_t idx) const {
    // Vertical alignment of 3 items: first item on the top edge - second item on centered (with text) - third item's label text on the bottom edge
    // Vertical alignment of 2 items: first item on the top edge - second item's label on the bottom edge
    // Vertical alignment of 1 item: centered with label
    // All centered (x-axis-wise)

    Rect16 rect = GetRect();
    // text is centered so width stays
    rect = Rect16::Height_t(icon_button_font_height); // labels height

    switch (std::min<size_t>(max_icons, GetBtnCount())) {
    case 1:
        rect += Rect16::Top_t(button_base_size + icon_label_delim);
        break;
    case 2:
        if (idx == 0) {
            rect += Rect16::Top_t(button_base_size + icon_label_delim);
        } else {
            rect += Rect16::Top_t(GetRect().Height() - icon_button_font_height);
        }
        break;
    case 3:
        if (idx == 0) {
            rect += Rect16::Top_t(button_base_size + icon_label_delim);
        } else if (idx == 1) {
            rect += Rect16::Top_t(GetRect().Height() / 2);
            rect -= Rect16::Top_t((button_base_size + icon_button_font_height + icon_label_delim) / 2);
            rect += Rect16::Top_t(button_base_size + icon_label_delim);
        } else {
            rect += Rect16::Top_t(GetRect().Height() - icon_button_font_height);
        }
        break;
    default:
        rect = Rect16();
        break;
    }
    return rect;
}

size_t IRadioButton::maxSize() const {
    return HasIcon() ? max_icons : max_buttons;
}

// 4th response for iconned layout is ensured to be _none
IRadioButton::Responses_t IRadioButton::generateResponses(const PhaseResponses &resp) const {
    Responses_t newResponses;
    newResponses[3] = Response::_none;
    for (size_t i = 0; i < maxSize(); ++i) {
        newResponses[i] = resp[i];
    }
    return newResponses;
};
