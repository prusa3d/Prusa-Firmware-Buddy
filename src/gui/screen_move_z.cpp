#include "screen_move_z.hpp"

#include <img_resources.hpp>
#include <marlin_client.hpp>
#include <common/marlin_vars.hpp>
#include <ScreenHandler.hpp>
#include <ScreenFactory.hpp>
#include <menu_vars.h>
#include <sound.hpp>
#include <utils/string_builder.hpp>
#include <gui/event/knob_event.hpp>

ScreenMoveZ::ScreenMoveZ()
    : screen_t()
    , value(static_cast<int32_t>(std::round(marlin_vars().logical_pos[2])))
    , lastQueuedPos(value)
    , axisText(this, text_rc, is_multiline::no, is_closed_on_click_t::no, _(axisLabel))
    , infoText(this, infoText_rc, is_multiline::yes, is_closed_on_click_t::no, _(infoTextContent))
    , closeText(this, closeText_rc, is_multiline::yes, is_closed_on_click_t::no, _(closeTextContent))
#if ENABLED(COREXY) // CoreXY moves bed down while Z goes up
    , rightText(this, rightText_rc, is_multiline::no, is_closed_on_click_t::no, _(downTextContent))
    , leftText(this, leftText_rc, is_multiline::no, is_closed_on_click_t::no, _(upTextContent))
#else /*PRINTER_TYPE*/
    , rightText(this, rightText_rc, is_multiline::no, is_closed_on_click_t::no, _(upTextContent))
    , leftText(this, leftText_rc, is_multiline::no, is_closed_on_click_t::no, _(downTextContent))
#endif /*PRINTER_TYPE*/
    , arrows(this, text_rc.TopRight(), { 0, 6, 0, 6 })
    , numb(this, numb_rc, value, "%d mm", font)
    , header(this, _(headerLabel))
    , icon(this, icon_rc, &img::turn_knob_81x55) {

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

void ScreenMoveZ::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK:
        close_screen();
        break;

    case GUI_event_t::KNOB: {
        auto &ctx = *static_cast<GuiEventContext *>(param);
        auto &ev = ctx.event.value<gui_event::KnobEvent>();
        const auto diff = ev.diff;

        int32_t val = diff + value;
        auto range = MenuVars::axis_range(Z_AXIS);
        val = std::clamp<int32_t>(val, range.first, range.second);

        value = val;
        numb.SetValue(value);
#if ENABLED(COREXY) // CoreXY moves bed down while Z goes up
        arrows.SetState(diff < 0 ? WindowArrows::State_t::up : WindowArrows::State_t::down);
#else /*PRINTER_TYPE*/
        arrows.SetState(diff < 0 ? WindowArrows::State_t::down : WindowArrows::State_t::up);
#endif /*PRINTER_TYPE*/

        ctx.accept();
        break;
    }

    case GUI_event_t::LOOP:
        if (value != lastQueuedPos) {
            ArrayStringBuilder<16> str_build; // We dont need decimal places here
            str_build.append_printf("G123 Z%.0f", (double)value);
            if (marlin_client::gcode_try(str_build.str()) == marlin_client::GcodeTryResult::Submitted) {
                lastQueuedPos = value;
            }
        }
        break;

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        sound::play(SoundType::button_echo);
        close_screen();
        break;

    default:
        screen_t::windowEvent(sender, event, param);
        break;
    }
}

void open_move_z_screen() {
    if (!Screens::Access()->IsScreenOpened(ScreenFactory::Screen<ScreenMoveZ>) && gui_get_nesting() <= 1) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenMoveZ>);
    }
}
