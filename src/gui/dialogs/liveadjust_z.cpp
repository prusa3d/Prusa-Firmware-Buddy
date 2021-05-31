// liveadjustz.cpp

#include "liveadjust_z.hpp"
#include "resource.h"
#include "sound.hpp"
#include "ScreenHandler.hpp"
#include "GuiDefaults.hpp"
#include "marlin_client.h"
#include "marlin_vars.h"
#include "eeprom.h"
#include "display_helper.h"

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
//WindowScale

WindowScale::WindowScale(window_t *parent, point_i16_t pt)
    : AddSuperWindow<window_frame_t>(parent, Rect16(pt, 10, 100))
    , scaleNum0(parent, getNumRect(pt), 0)
    , scaleNum1(parent, getNumRect(pt), -1)
    , scaleNum2(parent, getNumRect(pt), -2) {
    scaleNum0.SetFormat("% .0f");

    scaleNum0.rect -= Rect16::Top_t(8);
    scaleNum1.rect += Rect16::Top_t((rect.Height() / 2) - 8);
    scaleNum2.rect += Rect16::Top_t(rect.Height() - 8);
}

Rect16 WindowScale::getNumRect(point_i16_t pt) const {
    return Rect16(pt.x - 30, pt.y, 30, 20);
}

void WindowScale::SetMark(float percent) {
    int tmp_val = rect.TopLeft().y + (rect.Height() * percent);
    if (rect.Contain(point_ui16(rect.Left(), tmp_val))) {
        mark_old_y = mark_new_y;
        mark_new_y = static_cast<uint16_t>(tmp_val);
        if (mark_old_y != mark_new_y) {
            Invalidate();
        }
    }
}

void WindowScale::unconditionalDraw() {
    /// redraw old mark line
    display::DrawLine(
        point_ui16(rect.Left(), mark_old_y),
        point_ui16(rect.Left() + 10, mark_old_y),
        COLOR_BLACK);
    /// vertical line of scale
    display::DrawLine(
        point_ui16(rect.Left() + 5, rect.Top()),
        point_ui16(rect.Left() + 5, rect.Top() + rect.Height()),
        COLOR_WHITE);
    /// horizontal lines
    display::DrawLine( // top (0)
        point_ui16(rect.Left(), rect.Top()),
        point_ui16(rect.Left() + 10, rect.Top()),
        COLOR_WHITE);
    display::DrawLine( // -
        point_ui16(rect.Left() + 2, rect.Top() + (rect.Height() * .25F)),
        point_ui16(rect.Left() + 8, rect.Top() + (rect.Height() * .25F)),
        COLOR_WHITE);
    display::DrawLine( // middle (-1)
        point_ui16(rect.Left(), rect.Top() + (rect.Height() / 2)),
        point_ui16(rect.Left() + 10, rect.Top() + (rect.Height() / 2)),
        COLOR_WHITE);
    display::DrawLine( // -
        point_ui16(rect.Left() + 2, rect.Top() + (rect.Height() * .75F)),
        point_ui16(rect.Left() + 8, rect.Top() + (rect.Height() * .75F)),
        COLOR_WHITE);
    display::DrawLine( // bottom (-2)
        point_ui16(rect.Left() + 2, rect.Top() + rect.Height()),
        point_ui16(rect.Left() + 8, rect.Top() + rect.Height()),
        COLOR_WHITE);
    /// scale mark line
    display::DrawLine(
        point_ui16(rect.Left(), mark_new_y),
        point_ui16(rect.Left() + 10, mark_new_y),
        COLOR_ORANGE);
}
/*****************************************************************************/
//WindowLiveAdjustZ

WindowLiveAdjustZ::WindowLiveAdjustZ(window_t *parent, point_i16_t pt)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectScreenBody)
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

void WindowLiveAdjustZ_withText::Idle() {
    number.Shadow();
}

void WindowLiveAdjustZ_withText::Activate() {
    number.Unshadow();
}

bool WindowLiveAdjustZ_withText::IsActive() {
    return number.IsShadowed();
}

void WindowLiveAdjustZ_withText::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::ENC_UP:
    case GUI_event_t::ENC_DN:
        if (IsActive()) {
            return; //discard event
        }
        break;
    default:
        break;
    }
    SuperWindowEvent(sender, event, param);
}

/*****************************************************************************/
//LiveAdjustZ

static constexpr const padding_ui8_t textPadding = { 10, 5, 0, 0 };

LiveAdjustZ::LiveAdjustZ()
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreenBody)
    , text(this, getTextRect(), is_multiline::yes, is_closed_on_click_t::no)
    , nozzle_icon(this, getNozzleRect(), IDR_PNG_nozzle_shape_48px)
    , adjuster(this, { 75, 215 })
    , scale(this, { 45, 125 }) {

    /// using window_t 1bit flag
    flags.close_on_click = is_closed_on_click_t::yes;

    /// title text
    text.SetText(_("Adjust the nozzle height above the heatbed by turning the knob"));
    text.SetPadding(textPadding);

    /// set right position of the nozzle for our value
    moveNozzle();
}

const Rect16 LiveAdjustZ::getTextRect() {
    // make space for 4 rows of text rendered with the default GUI font
    return Rect16(0, 32, 240, 4 * GuiDefaults::Font->h + textPadding.bottom + textPadding.top);
}

const Rect16 LiveAdjustZ::getNozzleRect() {
    return Rect16(120 - 24, 120, 48, 48);
}

void LiveAdjustZ::moveNozzle() {
    uint16_t old_top = nozzle_icon.rect.Top();
    Rect16 moved_rect = getNozzleRect();                // starting position - 0%
    float percent = adjuster.GetValue() / z_offset_min; // z_offset value in percent

    // set move percent for a scale line indicator
    scale.SetMark(percent);

    moved_rect += Rect16::Top_t(int(40 * percent)); // how much will nozzle move
    nozzle_icon.rect = moved_rect;
    if (old_top != nozzle_icon.rect.Top()) {
        nozzle_icon.Invalidate();
    }
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
        /// but there is a problem with focus !!!parrent window of this dialog has it!!!
        if (flags.close_on_click == is_closed_on_click_t::yes)
            Screens::Access()->Close();
        break;
    default:
        SuperWindowEvent(sender, event, param);
    }
}

/// static
void LiveAdjustZ::Show() {
    LiveAdjustZ liveadjust;
    liveadjust.MakeBlocking();
}
