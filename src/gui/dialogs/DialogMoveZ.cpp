
#include "DialogMoveZ.hpp"
#include "screen_menu_move_utils.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "menu_vars.h"

bool DialogMoveZ::DialogShown = false;

DialogMoveZ::DialogMoveZ()
    : AddSuperWindow<IDialog>(GuiDefaults::EnableDialogBigLayout ? GuiDefaults::RectScreen : GuiDefaults::RectScreenNoFoot)
    , value(round(marlin_vars()->logical_pos[2]))
    , lastQueuedPos(value)
    , axisText(this, text_rc, is_multiline::no, is_closed_on_click_t::no, _(axisLabel))
    , infoText(this, infoText_rc, is_multiline::yes, is_closed_on_click_t::no, _(infoTextContent))
    , closeText(this, closeText_rc, is_multiline::no, is_closed_on_click_t::no, _(closeTextContent))
#if (PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX) // XL moves bed down while Z goes up
    , rightText(this, rightText_rc, is_multiline::no, is_closed_on_click_t::no, _(downTextContent))
    , leftText(this, leftText_rc, is_multiline::no, is_closed_on_click_t::no, _(upTextContent))
#else /*PRINTER_TYPE*/
    , rightText(this, rightText_rc, is_multiline::no, is_closed_on_click_t::no, _(upTextContent))
    , leftText(this, leftText_rc, is_multiline::no, is_closed_on_click_t::no, _(downTextContent))
#endif /*PRINTER_TYPE*/
    , arrows(this, text_rc.TopRight(), { 0, 6, 0, 6 })
    , numb(this, numb_rc, value, "%d mm", GuiDefaults::FontBig)
    , header(this, _(headerLabel))
    , icon(this, icon_rc, &img::turn_knob_81x55) {
    DialogShown = true;

    prev_accel = marlin_vars()->travel_acceleration;
    marlin_client::gcode("M204 T200");
    /// using window_t 1bit flag
    flags.close_on_click = is_closed_on_click_t::yes;
    header.SetIcon(&img::z_axis_16x16);

    constexpr static padding_ui8_t padding({ 6, 0, 6, 0 });

    //  info text
    infoText.SetPadding(padding);
    infoText.set_font(GuiDefaults::FontMenuSpecial);
    infoText.SetAlignment(Align_t::Center());

    //  close text
    closeText.SetPadding(padding);
    closeText.set_font(GuiDefaults::FontMenuSpecial);
    closeText.SetAlignment(Align_t::Center());

    //  UP DOWN texts
    rightText.set_font(GuiDefaults::FontBig);
    rightText.SetAlignment(Align_t::Left());
    leftText.set_font(GuiDefaults::FontBig);
    leftText.SetAlignment(Align_t::Right());

    // axis text
    axisText.set_font(GuiDefaults::FontBig);
    axisText.SetPadding({ 6, 0, 15, 0 });
    axisText.SetAlignment(Align_t::RightCenter());

    // numb
    numb.padding = { 15, 0, 6, 0 };
    numb.PrintAsInt32();
    numb.SetAlignment(Align_t::LeftCenter());

    // arrows
    arrows.SetState(WindowArrows::State_t::undef);
};

void DialogMoveZ::windowEvent(EventLock, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK: {
        /// has set is_closed_on_click_t
        /// todo
        /// GUI_event_t::CLICK could bubble into window_t::windowEvent and close dialog
        /// so CLICK could be left unhandled here
        /// but there is a problem with focus !!!parrent window of this dialog has it!!!
        if (flags.close_on_click == is_closed_on_click_t::yes) {
            Screens::Access()->Close();
        }
        return;
    }
    case GUI_event_t::ENC_DN: {
        const int enc_change = int(param);
        change(-enc_change);
        numb.SetValue(value);
#if (PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX) // XL moves bed down while Z goes up
        arrows.SetState(WindowArrows::State_t::up);
#else /*PRINTER_TYPE*/
        arrows.SetState(WindowArrows::State_t::down);
#endif /*PRINTER_TYPE*/
        return;
    }
    case GUI_event_t::ENC_UP: {
        const int enc_change = int(param);
        change(enc_change);
        numb.SetValue(value);
#if (PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_iX) // XL moves bed down while Z goes up
        arrows.SetState(WindowArrows::State_t::down);
#else /*PRINTER_TYPE*/
        arrows.SetState(WindowArrows::State_t::up);
#endif /*PRINTER_TYPE*/
        return;
    }
    case GUI_event_t::LOOP: {
        jog_axis(lastQueuedPos, value, Z_AXIS);
        return;
    }
    default:
        return;
    }
}

void DialogMoveZ::change(int diff) {
    int32_t val = diff + value;
    auto range = MenuVars::GetAxisRanges()[2];
    value = std::clamp(val, (int32_t)range[0], (int32_t)range[1]);
}

DialogMoveZ::~DialogMoveZ() {
    DialogShown = false;
    char msg[20];
    snprintf(msg, sizeof(msg), "M204 T%f", (double)prev_accel);
    marlin_client::gcode(msg);
}
void DialogMoveZ::Show() {
    // checking nesting to not open over some other blocking dialog
    // when blocking dialog is open, the nesting is larger than one
    if (!DialogShown && gui_get_nesting() <= 1) {
        DialogMoveZ moveZ;
        moveZ.MakeBlocking();
    }
}
