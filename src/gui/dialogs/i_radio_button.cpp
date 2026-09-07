#include "i_radio_button.hpp"

#include "sound.hpp"
#include "fonts.hpp"
#include "gui.hpp"
#include "display.hpp"
#include <gui/event/knob_event.hpp>
#include <client_response_texts.hpp>
#include <marlin_client.hpp>
#include <gui/gui_utils.hpp>

#include <algorithm> //find

static constexpr uint8_t button_delim_size = 31;
static constexpr uint8_t button_base_size = GuiDefaults::ButtonIconSize;
static constexpr uint8_t label_delim_size = 11;
static constexpr uint8_t label_base_size = button_base_size + 20;
static constexpr uint8_t icon_button_font_height = 16;
static constexpr uint8_t icon_label_delim = 5;

/*****************************************************************************/
// nonstatic variables and methods

IRadioButton::IRadioButton(window_t *parent, Rect16 rect)
    : window_t(parent, rect) {
    SetBackColor(COLOR_BRAND);
    SetBtnCount(0);
    SetBtnIndex(0);
    Enable();

    click_callback_ = [this](Response r) {
        if (GetParent()) {
            GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, event_conversion_union { .response = r }.pvoid);
        }
    };
}

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
IRadioButton::IRadioButton(window_t *parent, Rect16 rect, const PhaseResponses &resp)
    : IRadioButton(parent, rect) {
    Change(resp);
}
#endif

IRadioButton::IRadioButton(window_t *parent, Rect16 rect, Responses_t resp)
    : IRadioButton(parent, rect) {
    Change(resp);
}

IRadioButton::IRadioButton(window_t *parent, Rect16 rect, FSMAndPhase fsm_phase)
    : RadioButton(parent, rect) {
    set_fsm_and_phase(fsm_phase);
}

void IRadioButton::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (!GetParent()) {
        return;
    }

    switch (event) {

    case GUI_event_t::CLICK:
        click_callback_(Click());
        break;

    case GUI_event_t::KNOB: {
        auto &ctx = *static_cast<GuiEventContext *>(param);
        auto &ev = ctx.event.value<gui_event::KnobEvent>();
        const int new_index = std::clamp(GetBtnIndex() + ev.diff, 0, static_cast<int>(GetBtnCount()) - 1);

        // isIndexValid still needed after clamp - we can have "holes".
        // Bug: Is there a way to "skip" the hole? How?
        //   Fortunately, we don't have any dialogs that would have a hole in
        //   the middle, we always have holes on the right side. Still, keeping
        //   the check as a defensive measure.
        if (isIndexValid(new_index) && new_index != GetBtnIndex()) {
            SetBtnIndex(new_index);
            sound::play(SoundType::encoder_move);

            // Accept the event only if we've actually changed the button
            // If we're at the end of the radio, this allows the system to focus a different window
            ctx.accept();
        }
        return;
    }

    case GUI_event_t::TOUCH_CLICK: {
        std::optional<size_t> new_index = std::nullopt;

        size_t btn_count = GetBtnCount();
        switch (btn_count) {
        case 0:
            break;
        case 1:
            new_index = 0; // single button fils entire area, no need to test if it was clicked, just return it
            break;
        default: {
            event_conversion_union un { .pvoid = param };
            Layout layout = getNormalBtnRects(btn_count);
            for (uint8_t i = 0; i < btn_count; ++i) {
                // Intentionally do not check for Y coords - should be covered by the overal radio rect
                // Also some radio button override get_rect_for_touch, which should give more vertical tolerance
                if (un.point.x >= layout.splits[i].Left() && un.point.x < layout.splits[i].Right()) {
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
            // sound::play(SoundType::button_echo);

            // generate click and send it to itself
            // child class might handle it, if not GUI_event_t::CLICK from this switch will be called
            WindowEvent(this, GUI_event_t::CLICK, 0);
        }
    }
        return;

    case GUI_event_t::FOCUS_IN: {
        auto &ctx = *static_cast<GuiEventContext *>(param);
        auto &ev = ctx.event.value<gui_event::FocusInEvent>();

        const auto item_count = this->GetBtnCount();
        if (item_count == 0) {
            // No heuristics
            break;
        }

        using Reason = gui_event::FocusInEvent::Reason;
        switch (ev.reason) {

        case Reason::unspecified:
            break;

        case Reason::forward_focus_chain:
            SetBtnIndex(0);
            break;

        case Reason::reverse_focus_chain:
            SetBtnIndex(item_count - 1);
            break;
        }
        break;
    }

    case GUI_event_t::FOCUS_OUT: {
        // Unselect all buttons
        set_button_index_nocheck(this->GetBtnCount());
        break;
    }

    default:
        window_t::windowEvent(sender, event, param);
    }
}

void IRadioButton::screenEvent(window_t *sender, GUI_event_t event, void *const param) {
    switch (event) {

        // Touch swipe left/right = selecting the "back" response
    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT: {
        if (const auto i = IndexFromResponse(Response::Back); i.has_value()) {
            SetBtnIndex(*i);
            WindowEvent(this, GUI_event_t::CLICK, 0);
        }
        return;
    }

    default:
        break;
    }

    window_t::screenEvent(sender, event, param);
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
        display::fill_rect(GetRect(), GetParent()->GetBackColor());
    }
}

static constexpr auto ButtonFont = Font::big;

static void button_draw(Rect16 rc_btn, Color back_color, Color parent_color, const string_view_utf8 &text, bool is_selected);

// called internally, responses must exist
void IRadioButton::draw_1_btn() {
    const char *txt_to_print = get_response_text(responseFromIndex(0));
    button_draw(GetRect(), GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), _(txt_to_print),
        IsEnabled(0) && !disabled_drawing_selected);
}

// called internally, responses must exist
void IRadioButton::draw_n_btns(size_t btn_count) {
    const uint32_t MAX_TEXT_BUFFER = 128;
    Layout layout = getNormalBtnRects(btn_count);

    for (size_t i = 0; i < btn_count; ++i) {
        string_view_utf8 drawn = _(get_response_text(responseFromIndex(i)));
        char buffer[MAX_TEXT_BUFFER] = { 0 };
        if (layout.text_widths[i] > layout.splits[i].Width()) {
            uint32_t max_btn_label_text = layout.splits[i].Width() / width(ButtonFont);
            size_t length = std::min(max_btn_label_text, MAX_TEXT_BUFFER - 1);
            length = drawn.copyToRAM(buffer, length);
            buffer[length] = 0;
            drawn = string_view_utf8::MakeRAM(buffer);
        }
        if (responseFromIndex(i) != Response::_none) {
            button_draw(layout.splits[i], GetBackColor(), GetParent() ? GetParent()->GetBackColor() : GetBackColor(), drawn,
                GetBtnIndex() == i && IsEnabled(i) && !disabled_drawing_selected);
        }
    }
    Color spaces_clr = (GetBackColor() == COLOR_BRAND) ? COLOR_BLACK : COLOR_BRAND;
    for (size_t i = 0; i < btn_count - 1; ++i) {
        display::fill_rect(layout.spaces[i], spaces_clr);
    }
}

IRadioButton::Layout IRadioButton::getNormalBtnRects(size_t btn_count) const {
    Layout ret;
    static_assert(sizeof(btn_count) <= GuiDefaults::MAX_DIALOG_BUTTON_COUNT, "Too many IRadioButtons to draw.");

    for (size_t index = 0; index < btn_count; index++) {
        string_view_utf8 txt = _(get_response_text(responseFromIndex(index)));
        ret.text_widths[index] = width(ButtonFont) * static_cast<uint8_t>(txt.computeNumUtf8Chars());
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

Rect16 IRadioButton::get_rect_for_touch() const {
    static constexpr int extra = 64;

    Rect16 rect = GetRect();
    return Rect16(rect.Left(), rect.Top() - extra, rect.Width(), rect.Height() + extra);
}

void IRadioButton::DisableDrawingSelected() {
    disabled_drawing_selected = true;
}
void IRadioButton::EnableDrawingSelected() {
    disabled_drawing_selected = false;
}

static void button_draw(Rect16 rc_btn, Color back_color, Color parent_color, const string_view_utf8 &text, bool is_selected) {
    Color button_cl = is_selected ? back_color : COLOR_GRAY;
    Color text_cl = is_selected ? COLOR_BLACK : COLOR_WHITE;
    if (GuiDefaults::RadioButtonCornerRadius) {
        display::draw_rounded_rect(rc_btn, parent_color, button_cl, GuiDefaults::RadioButtonCornerRadius, MIC_ALL_CORNERS);
        rc_btn += Rect16::Left_t(GuiDefaults::RadioButtonCornerRadius);
        rc_btn -= Rect16::Width_t(2 * GuiDefaults::RadioButtonCornerRadius);
    }

    const Font font = auto_select_font(
        {
            .text = text,
            .rect = rc_btn,
            .largest = ButtonFont,
            .smallest = Font::small,
            .multiline = false,
        })
                          .value_or(Font::small);

    render_text_align(rc_btn, text, font, button_cl, text_cl, { 0, 0, 0, 0 }, Align_t::Center());
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

bool IRadioButton::isIndexValid(int index) {
    if (index < 0) {
        return false;
    }

    if (fixed_width_buttons_count > 0) {
        return (responseFromIndex(index) != Response::_none);
    } else {
        return index < GetBtnCount();
    }
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
    set_button_index_nocheck(index < GetBtnCount() ? index : 0);
}

void IRadioButton::set_button_index_nocheck(uint8_t idx) {
    if (idx != flags.class_specific.button_index) {
        flags.class_specific.button_index = idx;
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
// TODO: REMOVEME BFW-6028
IRadioButton::Responses_t IRadioButton::generateResponses(const PhaseResponses &resp) {
    Responses_t newResponses;
    newResponses[3] = Response::_none;
    for (size_t i = 0; i < max_buttons; ++i) {
        newResponses[i] = resp[i];
    }
    return newResponses;
};

std::optional<size_t> IRadioButton::IndexFromResponse(Response btn) const {
    for (size_t i = 0; i < maxSize(); ++i) {
        if (btn == responses[i]) {
            return i;
        }
    }
    return std::nullopt;
}

Response IRadioButton::responseFromIndex(size_t index) const {
    if (index >= maxSize()) {
        return Response::_none;
    }
    return responses[index];
}

void IRadioButton::Change(Responses_t resp) {
    if (responses == resp) {
        return;
    }
    responses = resp;
    SetBtnCount(fixed_width_buttons_count > 0 ? fixed_width_buttons_count : cnt_filled_responses(responses));

    // in iconned layout index will stay
    if (fixed_width_buttons_count == 0) {
        SetBtnIndex(0);
    }

    validateBtnIndex();

    invalidateWhatIsNeeded();
}

// TODO: REMOVEME completely BFW-6028
#if MAX_RESPONSES != 4
void IRadioButton::Change(const PhaseResponses &resp) {
    Change(generateResponses(resp));
}
#endif

void IRadioButton::set_fsm_and_phase(FSMAndPhase target) {
    set_fsm_and_phase(target, ClientResponses::get_fsm_responses(target.fsm, target.phase));
}

void IRadioButton::set_fsm_and_phase(FSMAndPhase target, PhaseResponses responses) {
    IRadioButton::Change(responses);
    click_callback_ = [target](Response r) {
        marlin_client::FSM_response(target, r);
    };
}
