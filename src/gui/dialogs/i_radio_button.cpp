#include "radio_button.hpp"
#include "ScreenHandler.hpp"
#include "sound.hpp"
#include "fonts.hpp" //IDR_FNT_BIG
#include "gui.hpp"

#include <algorithm> //find

static constexpr uint8_t button_delim_size = 31;
static constexpr uint8_t button_base_size = GuiDefaults::ButtonIconSize;
static constexpr uint8_t label_delim_size = 11;
static constexpr uint8_t label_base_size = button_base_size + 20;
static constexpr uint8_t icon_button_font_height = 16;
static constexpr uint8_t icon_label_delim = 5;

/*****************************************************************************/
// static variables and methods
static const IRadioButton::Responses_t no_responses = { Response::_none, Response::_none, Response::_none, Response::_none }; // used in constructor

size_t IRadioButton::cnt_labels(const PhaseTexts *labels) {
    if (!labels) {
        return 0;
    }
    return (std::find_if(labels->begin(), labels->end(), [](const char *s) { return s[0] == '\0'; })) - labels->begin();
}

size_t IRadioButton::cnt_responses(Responses_t resp) {
    return cnt_filled_responses(resp);
}

size_t IRadioButton::cnt_buttons(const PhaseTexts *labels, Responses_t resp) {
    size_t lbls = std::min(cnt_labels(labels), max_buttons);
    size_t cmds = std::min(cnt_responses(resp), max_buttons);
    return std::max(lbls, cmds);
}

/*****************************************************************************/
// nonstatic variables and methods

IRadioButton::IRadioButton(window_t *parent, Rect16 rect, size_t count)
    : AddSuperWindow<window_t>(parent, rect)
    , pfont(resource_font(IDR_FNT_BIG)) {
    SetBackColor(COLOR_ORANGE);
    SetBtnCount(count);
    SetBtnIndex(0);
    Enable();
}

// no overflow
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

// no underflow
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
    if (!GetParent()) {
        return;
    }

    switch (event) {
    case GUI_event_t::CLICK: {
        // send response to parent via GUI_event_t::CHILD_CLICK
        Response response = Click();
        event_conversion_union un;
        un.response = response;
        if (GetParent()) {
            GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, un.pvoid);
        }
    } break;
    case GUI_event_t::ENC_UP:
        ++(*this);
        return;
    case GUI_event_t::ENC_DN:
        --(*this);
        return;
    case GUI_event_t::TOUCH: {
        event_conversion_union un;
        un.pvoid = param;
        std::optional<size_t> new_index = std::nullopt;

        size_t btn_count = GetBtnCount();
        switch (btn_count) {
        case 0:
            break;
        case 1:
            new_index = 0; // single button fils entire area, no need to test if it was clicked, just return it
            break;
        default: {
            Layout layout = getNormalBtnRects(btn_count);
            for (uint8_t i = 0; i < btn_count; ++i) {
                if (layout.splits[i].Contain(un.point)) {
                    new_index = i;
                    break;
                }
            }
        }
        }

        if (new_index) {
            // select button
            SetBtnIndex(*new_index);

            // generate click sound??
            // Sound_Play(eSOUND_TYPE::ButtonEcho);

            // generate click and send it to itself
            // child class might handle it, if not GUI_event_t::CLICK from this switch will be called
            WindowEvent(this, GUI_event_t::CLICK, param);
        }
    }
        return;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

void IRadioButton::unconditionalDraw() {
    const size_t cnt = GetBtnCount();
    switch (cnt) {
    case 0:
        draw_0_btn(); // cannot use draw_n_btns, would div by 0
        break;
    case 1:
        draw_1_btn(); // could use draw_n_btns, but this is much faster
        break;
    default:
        draw_n_btns(cnt);
        break;
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
    button_draw(GetRect(), GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), _(txt_to_print), pfont,
        IsEnabled(0) && !disabled_drawing_selected);
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
        if (responseFromIndex(i) != Response::_none) {
            button_draw(layout.splits[i], GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), drawn, pfont,
                GetBtnIndex() == i && IsEnabled(i) && !disabled_drawing_selected);
        }
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
    GetRect().HorizontalSplit(
        ret.splits,
        ret.spaces,
        btn_count,
        GuiDefaults::ButtonSpacing,
        // For fixed width buttons, disregard text lengths (assuming all texts will fit)
        fixed_width_buttons_count == 0 ? ret.text_widths : nullptr);

    return ret;
}

void IRadioButton::DisableDrawingSelected() {
    disabled_drawing_selected = true;
}
void IRadioButton::EnableDrawingSelected() {
    disabled_drawing_selected = false;
}

void IRadioButton::button_draw(Rect16 rc_btn, color_t back_color, color_t parent_color, string_view_utf8 text, const font_t *pf, bool is_selected) {
    color_t button_cl = is_selected ? back_color : COLOR_GRAY;
    color_t text_cl = is_selected ? COLOR_BLACK : COLOR_WHITE;
    if (GuiDefaults::RadioButtonCornerRadius) {
        display::DrawRoundedRect(rc_btn, parent_color, button_cl, GuiDefaults::RadioButtonCornerRadius, MIC_ALL_CORNERS);
        rc_btn += Rect16::Left_t(GuiDefaults::RadioButtonCornerRadius);
        rc_btn -= Rect16::Width_t(2 * GuiDefaults::RadioButtonCornerRadius);
    }
    render_text_align(rc_btn, text, pf, button_cl, text_cl, { 0, 0, 0, 0 }, Align_t::Center());
}

bool IRadioButton::IsEnabled(size_t index) const {
    return responseFromIndex(index) != Response::_none;
}

/**
 * @brief more advanced validation meant pimary for icons
 * iconned layout support to first or second response to be _none
 * if it is focused, focus must shift
 * if no valid response is found, index shall be 0
 */
void IRadioButton::validateBtnIndex() {
    if (isIndexValid(GetBtnIndex())) {
        return; // index valid
    }

    SetBtnIndex(0);

    if (fixed_width_buttons_count > 0) {
        if (isIndexValid(GetBtnIndex())) {
            return;
        }

        for (size_t i = 0; i < fixed_width_buttons_count; ++i) {
            if (responseFromIndex(i) != Response::_none) {
                SetBtnIndex(i);
                return;
            }
        }
    }
}

bool IRadioButton::isIndexValid(size_t index) {
    if (fixed_width_buttons_count > 0) {
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
    if (fixed_width_buttons_count > 0 && HasValidBackground()) {
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
    if (index) {
        SetBtnIndex(*index);
    }
}

size_t IRadioButton::maxSize() const {
    return fixed_width_buttons_count > 0 ? fixed_width_buttons_count : max_buttons;
}

// 4th response for iconned layout is ensured to be _none
IRadioButton::Responses_t IRadioButton::generateResponses(const PhaseResponses &resp) {
    Responses_t newResponses;
    newResponses[3] = Response::_none;
    for (size_t i = 0; i < max_buttons; ++i) {
        newResponses[i] = resp[i];
    }
    return newResponses;
};
