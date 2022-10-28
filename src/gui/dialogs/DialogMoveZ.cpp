
#include "DialogMoveZ.hpp"
#include "ScreenHandler.hpp"
#include "marlin_client.h"
#include "menu_vars.h"
#include "resource.h"

bool DialogMoveZ::DialogShown = false;

DialogMoveZ::DialogMoveZ()
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreenNoFoot)
    , value(marlin_vars()->pos[2])
    , lastQueuedPos(marlin_vars()->pos[2])
    , axisText(this, text_rc, is_multiline::no, is_closed_on_click_t::no, _(axisLabel))
    , infoText(this, infoText_rc, is_multiline::yes, is_closed_on_click_t::no, _(infoTextContent))
    , closeText(this, closeText_rc, is_multiline::no, is_closed_on_click_t::no, _(closeTextContent))
    , upText(this, upText_rc, is_multiline::no, is_closed_on_click_t::no, _(upTextContent))
    , downText(this, downText_rc, is_multiline::no, is_closed_on_click_t::no, _(downTextContent))
    , arrows(this, text_rc.TopRight(), { 0, 6, 0, 6 })
    , numb(this, numb_rc, marlin_vars()->pos[2], "%d mm", GuiDefaults::FontBig)
    , header(this, _(headerLabel))
    , icon(this, icon_rc, PNG::turn_knob_81x55) {
    DialogShown = true;

    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_TRAVEL_ACCEL));
    prev_accel = marlin_vars()->travel_acceleration;
    marlin_gcode("M204 T200");
    /// using window_t 1bit flag
    flags.close_on_click = is_closed_on_click_t::yes;
    header.SetIcon(png::Get<png::Id::z_axis_16x16>());

    constexpr static padding_ui8_t padding({ 6, 0, 6, 0 });

    //  info text
    infoText.SetPadding(padding);
    infoText.font = GuiDefaults::FontMenuSpecial;
    infoText.SetAlignment(Align_t::Center());

    //  close text
    closeText.SetPadding(padding);
    closeText.font = GuiDefaults::FontMenuSpecial;
    closeText.SetAlignment(Align_t::Center());

    upText.font = GuiDefaults::FontBig;

    downText.font = GuiDefaults::FontBig;

    // axis text
    axisText.font = GuiDefaults::FontBig;
    axisText.SetPadding({ 6, 0, 15, 0 });
    axisText.SetAlignment(Align_t::RightCenter());

    // numb
    numb.padding = { 15, 0, 6, 0 };
    numb.PrintAsInt32();
    numb.SetAlignment(Align_t::LeftCenter());

    // arrows
    arrows.SetState(WindowArrows::State_t::undef);
};

void DialogMoveZ::windowEvent(EventLock, window_t *sender, GUI_event_t event, void *param) {
    constexpr static uint8_t len = 4;

    switch (event) {
    case GUI_event_t::CLICK: {
        /// has set is_closed_on_click_t
        /// todo
        /// GUI_event_t::CLICK could bubble into window_t::windowEvent and close dialog
        /// so CLICK could be left unhandled here
        /// but there is a problem with focus !!!parrent window of this dialog has it!!!
        if (flags.close_on_click == is_closed_on_click_t::yes)
            Screens::Access()->Close();
        return;
    }
    case GUI_event_t::ENC_DN: {
        const int enc_change = int(param);
        change(-enc_change);
        numb.SetValue(value);
        arrows.SetState(WindowArrows::State_t::down);
        return;
    }
    case GUI_event_t::ENC_UP: {
        const int enc_change = int(param);
        change(enc_change);
        numb.SetValue(value);
        arrows.SetState(WindowArrows::State_t::up);
        return;
    }
    case GUI_event_t::LOOP: {

        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE));
        if (marlin_vars()->pqueue <= len) {
            int difference = value - lastQueuedPos;
            uint8_t freeSlots = len - marlin_vars()->pqueue;
            if (difference != 0) {
                for (uint8_t i = 0; i < freeSlots && lastQueuedPos != (int)value; i++) {
                    if (difference > 0) {
                        lastQueuedPos++;
                        difference--;
                    } else if (difference < 0) {
                        lastQueuedPos--;
                        difference++;
                    }
                    marlin_move_axis(lastQueuedPos, MenuVars::GetManualFeedrate()[2], 2);
                }
            }
        }
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
    marlin_gcode(msg);
}
void DialogMoveZ::Show() {
    // checking nesting to not open over some other blocking dialog
    // when blocking dialog is open, the nesting is larger than one
    if (!DialogShown && gui_get_nesting() <= 1) {
        DialogMoveZ moveZ;
        moveZ.MakeBlocking();
    }
}
