/**
 * @file radio_button_preview.cpp
 */

#include "radio_button_preview.hpp"
#include "window_icon.hpp"
#include "window_text.hpp"
#include "i18n.h"
#include "img_resources.hpp"
#include <client_response.hpp>
#include <tools_mapping.hpp>

const char *label_texts[] = {
    N_("Print"),
    N_("Back")
};

static constexpr const img::Resource *icons[] = {
    &img::print_80x80,
    &img::back_80x80,
    &img::print_80x80_focused,
    &img::back_80x80_focused
};

static constexpr const uint8_t icon_label_delim = 5;
static constexpr const uint8_t label_height = 16;
static constexpr const uint8_t button_cnt = 2;

RadioButtonPreview::RadioButtonPreview(window_t *parent, Rect16 rect)
    : RadioButtonFSM(parent, rect, PhasesPrintPreview::main_dialog) {
}

Rect16 RadioButtonPreview::getVerticalIconRect(uint8_t idx) const {
    Rect16 rect = GetRect();
    return Rect16(
        rect.Left() + (rect.Width() - GuiDefaults::ButtonIconSize) / 2,
        rect.Top() + idx * (GuiDefaults::ButtonIconSize + GuiDefaults::ButtonIconVerticalSpacing),
        GuiDefaults::ButtonIconSize,
        GuiDefaults::ButtonIconSize);
}

Rect16 RadioButtonPreview::getVerticalLabelRect(uint8_t idx) const {
    Rect16 rect = GetRect();
    return Rect16(
        rect.Left(),
        rect.Top() + GuiDefaults::ButtonIconSize + icon_label_delim + idx * (GuiDefaults::ButtonIconSize + GuiDefaults::ButtonIconVerticalSpacing),
        rect.Width(),
        label_height);
}

void RadioButtonPreview::unconditionalDraw() {
    for (int i = 0; i < button_cnt; i++) {
        uint8_t res_offset = GetBtnIndex() == i ? button_cnt : 0;
        window_icon_t icon(nullptr, getVerticalIconRect(i), icons[res_offset + i]);
        window_text_t label(nullptr, getVerticalLabelRect(i), is_multiline::no, is_closed_on_click_t::no, _(label_texts[i]));

        if (ClientResponses::GetResponse(PhasesPrintPreview::main_dialog, i) == Response::Continue) {
            if (tools_mapping::is_tool_mapping_possible()) {
                label.SetText(_("Continue")); // replace print with continue if tools mapping will show
                icon.SetRes(res_offset ? &img::mapping_80x80_focused : &img::mapping_80x80); // replace icon with tools mapping one
            }
        }

        label.set_font(Font::small);
        label.SetAlignment(Align_t::Center());

        icon.Draw();
        label.Draw();
    }
}

void RadioButtonPreview::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK: {
        const Response response = Click();

        marlin_client::FSM_response(fsm_and_phase(), response);

        if (GetParent()) {
            GetParent()->WindowEvent(this, GUI_event_t::CHILD_CLICK, event_conversion_union { .response = response }.pvoid);
        }
    } break;

    case GUI_event_t::TOUCH_CLICK: {
        event_conversion_union un;
        un.pvoid = param;
        std::optional<size_t> new_index = std::nullopt;
        for (size_t i = 0; i < button_cnt; ++i) {
            if (getVerticalIconRect(i).Contain(un.point)) {
                new_index = i;
                break;
            }
        }

        if (new_index) {
            SetBtnIndex(*new_index);
            // Sound_Play(eSOUND_TYPE::ButtonEcho);
            // Generate click for itself
            WindowEvent(this, GUI_event_t::CLICK, param);
        }
    } break;

    default:
        RadioButtonFSM::windowEvent(sender, event, param);
        break;
    }
}
