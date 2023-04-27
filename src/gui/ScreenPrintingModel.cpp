/**
 * @file ScreenPrintingModel.cpp
 */
#include "ScreenPrintingModel.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "screen_printing_layout.hpp"

static constexpr uint16_t btn_size = GuiDefaults::ButtonIconSize;
static constexpr uint16_t btn_y_offset = 185;
static constexpr uint16_t btn_text_spacing = 5;
static constexpr uint16_t text_height = 17;

template <class T>
static constexpr auto get_index(T enum_idx) {
    return ftrstd::to_underlying(enum_idx);
}

static constexpr const char *label_resources[] = {
    N_("Tune"),
    N_("Pause"),
    N_("Pausing..."),
    N_("Stop"),
    N_("Resume"),
    N_("Resuming..."),
    N_("Heating..."),
    N_("Reprint"),
    N_("Home"),
    N_("Skip"),
    N_("Disconnect")
};

ScreenPrintingModel::ScreenPrintingModel(string_view_utf8 caption)
    : AddSuperWindow<IScreenPrinting>(caption)
    // clang-format off
    , buttons {
        {this, GetButtonRect(0), &icon_resources[get_index(BtnRes::Settings)], TuneAction},
        {this, GetButtonRect(1), &icon_resources[get_index(BtnRes::Pause)], PauseAction},
        {this, GetButtonRect(2), &icon_resources[get_index(BtnRes::Stop)], StopAction}
    }
    , labels {
        {this, GetButtonLabelRect(0), is_multiline::no, is_closed_on_click_t::no, _(label_resources[get_index(LabelRes::Settings)])},
        {this, GetButtonLabelRect(1), is_multiline::no, is_closed_on_click_t::no, _(label_resources[get_index(LabelRes::Pause)])},
        {this, GetButtonLabelRect(2), is_multiline::no, is_closed_on_click_t::no, _(label_resources[get_index(LabelRes::Stop)])}
    } // clang-format on
{
    for (uint8_t i = 0; i < socket_count; i++) {
        labels[i].font = resource_font(IDR_FNT_SMALL);
        labels[i].SetPadding({ 0, 0, 0, 0 });
        labels[i].SetAlignment(Align_t::Center());
    }
}

Rect16 ScreenPrintingModel::GetButtonRect(uint8_t idx) {
    return Rect16(btn_padding + (btn_size + btn_spacing) * idx, btn_y_offset, btn_size, btn_size);
}

Rect16 ScreenPrintingModel::GetButtonLabelRect(uint8_t idx) {
    return Rect16(btn_padding - btn_spacing / 2 + (btn_size + btn_spacing) * idx, btn_y_offset + btn_size + btn_text_spacing, btn_size + btn_spacing, text_height);
}

void ScreenPrintingModel::SetButtonIconAndLabel(BtnSocket idx, BtnRes ico_res, LabelRes txt_res) {
    SetButtonIcon(idx, ico_res);
    SetButtonLabel(idx, txt_res);
}

void ScreenPrintingModel::SetButtonIcon(BtnSocket idx, BtnRes ico_res) {
    if (idx > BtnSocket::_last || ico_res > BtnRes::_last)
        return;
    buttons[get_index(idx)].SetRes(&icon_resources[get_index(ico_res)]);
}

void ScreenPrintingModel::SetButtonLabel(BtnSocket idx, LabelRes txt_res) {
    if (idx > BtnSocket::_last || txt_res > LabelRes::_last)
        return;
    labels[get_index(idx)].SetText(_(label_resources[get_index(txt_res)]));
}

void ScreenPrintingModel::DisableButton(BtnSocket idx) {
    const size_t btn_idx = get_index(idx);
    if (idx > BtnSocket::_last || buttons[btn_idx].IsShadowed())
        return;

    // Move focus to the right when disabled - if any button is enabled
    if (buttons[btn_idx].IsFocused()) {
        for (size_t i = 1; i < socket_count; i++) {
            if (!buttons[(btn_idx + i) % socket_count].IsShadowed()) {
                buttons[(btn_idx + i) % socket_count].SetFocus();
                break;
            }
        }
    }

    buttons[btn_idx].Shadow();
    buttons[btn_idx].Disable();
    buttons[btn_idx].Invalidate();
}

void ScreenPrintingModel::EnableButton(BtnSocket idx) {
    const size_t btn_idx = get_index(idx);
    if (idx > BtnSocket::_last || !buttons[btn_idx].IsShadowed())
        return;

    buttons[btn_idx].Unshadow();
    buttons[btn_idx].Enable();
    buttons[btn_idx].Invalidate();
}
