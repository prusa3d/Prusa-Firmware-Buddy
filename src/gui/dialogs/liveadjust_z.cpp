// liveadjustz.cpp

#include "liveadjust_z.hpp"
#include "resource.h"
#include "sound.hpp"
#include "ScreenHandler.hpp"
#include "GuiDefaults.hpp"
#include "marlin_client.h"
#include "marlin_vars.h"
#include "display_helper.h"
#include "SteelSheets.hpp"

#include "config_features.h"
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "gui_config_mini.h"
#else
    #error "Unknown PRINTER_TYPE."
#endif

const int axis_steps_per_unit[] = DEFAULT_AXIS_STEPS_PER_UNIT;
const float z_offset_step = 1.0F / float(axis_steps_per_unit[2]);
constexpr float z_offset_min = SteelSheets::zOffsetMin;
constexpr float z_offset_max = SteelSheets::zOffsetMax;

/*****************************************************************************/
//WindowScale

WindowScale::WindowScale(window_t *parent, point_i16_t pt)
    : AddSuperWindow<window_frame_t>(parent, Rect16(pt, 10, 100))
    , scaleNum0(parent, getNumRect(pt), 0)
    , scaleNum1(parent, getNumRect(pt), -1)
    , scaleNum2(parent, getNumRect(pt), -2) {
    scaleNum0.SetFormat("% .0f");

    scaleNum0 -= Rect16::Top_t(8);
    scaleNum1 += Rect16::Top_t((Height() / 2) - 8);
    scaleNum2 += Rect16::Top_t(Height() - 8);
}

Rect16 WindowScale::getNumRect(point_i16_t pt) const {
    return Rect16(pt.x - 30, pt.y, 30, 20);
}

void WindowScale::SetMark(float percent) {
    if (!mark_old_y)
        mark_old_y = mark_new_y;
    mark_new_y = Height() * std::clamp(percent, 0.f, 100.f);
    if (mark_old_y != mark_new_y) {
        Invalidate();
    } else {
        Validate();
    }
}

void WindowScale::horizLine(uint16_t width_pad, uint16_t height, color_t color) {
    display::DrawLine(
        point_ui16(Left() + width_pad, Top() + height),
        point_ui16(Left() + 10 - width_pad, Top() + height),
        color);
}

void WindowScale::unconditionalDraw() {
    /// redraw old mark line
    if (mark_old_y)
        horizLine(0, *mark_old_y, COLOR_BLACK);
    mark_old_y = std::nullopt;
    /// vertical line of scale
    display::DrawLine(point_ui16(Left() + 5, Top()), point_ui16(Left() + 5, Top() + Height()), COLOR_WHITE);
    /// horizontal lines
    horizLineWhite(0, 0);
    horizLineWhite(2, Height() / 4);
    horizLineWhite(0, Height() / 2);
    horizLineWhite(2, Height() / 4 * 3);
    horizLineWhite(0, Height());
    /// scale mark line
    horizLine(0, mark_new_y, COLOR_ORANGE);
}
/*****************************************************************************/
//WindowLiveAdjustZ

WindowLiveAdjustZ::WindowLiveAdjustZ(window_t *parent, point_i16_t pt)
    : AddSuperWindow<window_frame_t>(parent, GuiDefaults::RectScreenBody)
    , number(this, getNumberRect(pt), marlin_vars()->z_offset)
    , arrows(this, getIconPoint(pt)) {

    SetRect(number.GetRect().Union(arrows.GetRect()));
    /// using window_numb to store float value of z_offset
    /// we have to set format and bigger font
    number.SetFont(GuiDefaults::FontBig);
    number.SetFormat("% .3f");
}

void WindowLiveAdjustZ::Save() {
    /// store new z offset value into a marlin_vars & EEPROM
    if (!SteelSheets::SetZOffset(number.GetValue())) {
        // could be during print, better not cause BSOD, just freeze in debug
        assert(0 /* Z offset write failed */);
    }
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
        break;
    case GUI_event_t::ENC_DN:
        Change(-1);
        Sound_Play(eSOUND_TYPE::EncoderMove);
        arrows.SetState(WindowArrows::State_t::down);
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
    Shift(ShiftDir_t::Right, width - Width());
    text.SetRect(Rect16(pt, width - Width(), Height()));
    SetRect(GetRect().Union(text.GetRect()));
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
    uint16_t old_top = nozzle_icon.Top();
    Rect16 moved_rect = getNozzleRect();                // starting position - 0%
    float percent = adjuster.GetValue() / z_offset_min; // z_offset value in percent

    // set move percent for a scale line indicator
    scale.SetMark(percent);

    moved_rect += Rect16::Top_t(int(40 * percent)); // how much will nozzle move
    nozzle_icon.SetRect(moved_rect);
    if (old_top != nozzle_icon.Top()) {
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
