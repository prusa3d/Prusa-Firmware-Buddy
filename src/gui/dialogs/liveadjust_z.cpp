// liveadjustz.cpp

#include "liveadjust_z.hpp"
#include "resource.h"
#include "sound.hpp"
#include "ScreenHandler.hpp"
#include "GuiDefaults.hpp"
#include "marlin_client.h"
#include "marlin_vars.h"
#include "eeprom.h"

#include "../Marlin/src/inc/MarlinConfig.h"
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "gui_config_mini.h"
    #include "Configuration_A3ides_2209_MINI_adv.h"
#else
    #error "Unknown PRINTER_TYPE."
#endif

const int axis_steps_per_unit[] = DEFAULT_AXIS_STEPS_PER_UNIT;
const float z_offset_step = 1.0F / float(axis_steps_per_unit[2]);
const float z_offset_min = Z_OFFSET_MIN;
const float z_offset_max = Z_OFFSET_MAX;

/*****************************************************************************/
//WindowLiveAdjustZ

WindowLiveAdjustZ::WindowLiveAdjustZ(window_t *parent, point_i16_t pt)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectScreenBody) //calculate size later
    , number(this, getNumberRect(pt), marlin_vars()->z_offset)
    , arrows(this, getIconPoint(pt)) {

    rect = number.rect.Union(arrows.rect);
    /// using window_numb to store float value of z_offset
    /// we have to set format and bigger font
    number.SetFont(GuiDefaults::FontBig);
    number.SetFormat("% .3f");
}

void WindowLiveAdjustZ::Save() {
    /// store new z offset value into a marlin_vars & EEPROM
    variant8_t var = variant8_flt(number.GetValue());
    eeprom_set_var(EEVAR_ZOFFSET, var);
    marlin_set_var(MARLIN_VAR_Z_OFFSET, var);
    /// force update marlin vars
    marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET));
}

void WindowLiveAdjustZ::Change(int dif) {
    float old = number.GetValue();
    float z_offset = number.value;

    z_offset += (float)dif * z_offset_step;
    z_offset = dif >= 0 ? std::max(z_offset, old) : std::min(z_offset, old); //check overflow/underflow
    z_offset = std::min(z_offset, z_offset_max);
    z_offset = std::max(z_offset, z_offset_min);

    /// check if value has changed so we are free to set babystep and store new value
    float baby_step = z_offset - old;
    if (baby_step != 0.0F) {
        number.SetValue(z_offset);
        marlin_do_babysteps_Z(baby_step);
    }
}

void WindowLiveAdjustZ::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::ENC_UP:
        Change(1);
        Sound_Play(eSOUND_TYPE::EncoderMove);
        arrows.SetState(WindowArrows::State_t::up);
        gui_invalidate();
        break;
    case GUI_event_t::ENC_DN:
        Change(-1);
        Sound_Play(eSOUND_TYPE::EncoderMove);
        arrows.SetState(WindowArrows::State_t::down);
        gui_invalidate();
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

/*****************************************************************************/
//WindowLiveAdjustZ_withText
WindowLiveAdjustZ_withText::WindowLiveAdjustZ_withText(window_t *parent, point_i16_t pt, size_t width)
    : AddSuperWindow<WindowLiveAdjustZ>(parent, pt)
    , text(parent, Rect16(), is_multiline::no, is_closed_on_click_t::no, _(text_str)) {
    Shift(ShiftDir_t::Right, width - rect.Width());
    text.rect = Rect16(pt, width - rect.Width(), rect.Height());
    rect = rect.Union(text.rect);
}

/*****************************************************************************/
//LiveAdjustZ

LiveAdjustZ::LiveAdjustZ(is_closed_on_click_t outside_close)
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreenBody)
    , text(this, getTextRect(), is_multiline::yes, is_closed_on_click_t::no)
    , nozzle_icon(this, getNozzleRect(), IDR_PNG_big_nozzle)
    , bed(this, Rect16(70, 180, 100, 10))
    , adjuster(this, { 75, 205 }) {

    /// using window_t 1bit flag
    flag_close_on_click = outside_close;

    /// simple rectangle as bed with defined background color
    bed.SetBackColor(COLOR_ORANGE);

    /// title text
    constexpr static const char *txt = N_("Adjust the nozzle height above the heatbed by turning the knob");
    static const string_view_utf8 text_view = _(txt);
    text.SetText(text_view);

    /// set right position of the nozzle for our value
    moveNozzle();
}

const Rect16 LiveAdjustZ::getTextRect() {
    return Rect16(0, 32, 240, 60);
}

const Rect16 LiveAdjustZ::getNozzleRect() {
    return Rect16(120 - 24, 120, 48, 48);
}

void LiveAdjustZ::moveNozzle() {
    float percent = adjuster.GetValue() / z_offset_min;
    Rect16 moved_rect = getNozzleRect();
    moved_rect += Rect16::Top_t(int(10 * percent));
    nozzle_icon.rect = moved_rect;
    nozzle_icon.Invalidate();
}

void LiveAdjustZ::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::ENC_UP:
    case GUI_event_t::ENC_DN:
        adjuster.WindowEvent(sender, event, param);
        Sound_Play(eSOUND_TYPE::EncoderMove);
        moveNozzle();
        gui_invalidate();
        break;
    case GUI_event_t::CLICK:
        /// has set is_closed_on_click_t
        /// destructor of WindowLiveAdjustZ stores new z offset value into a marlin_vars & EEPROM
        /// todo
        /// GUI_event_t::CLICK could bubble into window_t::windowEvent and close dialog
        /// so CLICK could be left unhandled here
        /// but there is a problem with focus !!!parrent window of this dialog has is!!!
        if (flag_close_on_click == is_closed_on_click_t::yes)
            Screens::Access()->Close();
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

/// static
void LiveAdjustZ::Open(is_closed_on_click_t outside_close) {
    LiveAdjustZ liveadjust(outside_close);
    liveadjust.MakeBlocking();
}
