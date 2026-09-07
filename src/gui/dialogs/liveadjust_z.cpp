// liveadjustz.cpp

#include "liveadjust_z.hpp"
#include <gui/event/gui_event.hpp>
#include "sound.hpp"
#include "ScreenHandler.hpp"
#include <ScreenFactory.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include "marlin_client.hpp"
#include "display.hpp"
#include "display_helper.h"
#include "img_resources.hpp"
#include <option/has_sheet_profiles.h>
#include "config_features.h"
#include <gui/event/knob_event.hpp>
#include <guiconfig/guiconfig.h>
#include <menu_vars.h>
#include <config_store/store_c_api.h>
#include <gui.hpp>

#if HAS_SHEET_PROFILES()
    #include "SteelSheets.hpp"
#endif

/*****************************************************************************/
// WindowScale

WindowScale::WindowScale(window_t *parent, point_i16_t pt)
    : window_frame_t(parent, Rect16(pt, 10, 100))
    , scaleNum0(parent, getNumRect(pt), z_offset_max, "% d")
    , scaleNum1(parent, getNumRect(pt), (z_offset_max + z_offset_min) / 2, "% d")
    , scaleNum2(parent, getNumRect(pt), z_offset_min, "% d") {

    scaleNum0 -= Rect16::Top_t(8);
    scaleNum1 += Rect16::Top_t((Height() / 2) - 8);
    scaleNum2 += Rect16::Top_t(Height() - 8);
    scaleNum0.PrintAsInt32();
    scaleNum1.PrintAsInt32();
    scaleNum2.PrintAsInt32();
}

Rect16 WindowScale::getNumRect(point_i16_t pt) const {
    return Rect16(pt.x - 30, pt.y, 30, 20);
}

void WindowScale::SetMark(float relative) {
    if (!mark_old_y) {
        mark_old_y = mark_new_y;
    }
    mark_new_y = static_cast<uint16_t>(Height() * std::clamp(relative, 0.f, 1.f));
    if (mark_old_y != mark_new_y) {
        Invalidate();
    } else {
        Validate();
    }
}

void WindowScale::horizLine(uint16_t width_pad, uint16_t height, Color color) {
    display::draw_line(
        point_ui16(Left() + width_pad, Top() + height),
        point_ui16(Left() + 10 - width_pad, Top() + height),
        color);
}

void WindowScale::unconditionalDraw() {
    /// redraw old mark line
    if (mark_old_y) {
        horizLine(0, *mark_old_y, COLOR_BLACK);
    }
    mark_old_y = std::nullopt;
    /// vertical line of scale
    display::draw_line(point_ui16(Left() + 5, Top()), point_ui16(Left() + 5, Top() + Height()), COLOR_WHITE);
    /// horizontal lines
    horizLineWhite(0, 0);
    horizLineWhite(2, Height() / 4);
    horizLineWhite(0, Height() / 2);
    horizLineWhite(2, Height() / 4 * 3);
    horizLineWhite(0, Height());
    /// scale mark line
    horizLine(0, mark_new_y, COLOR_BRAND);
}
/*****************************************************************************/
// WindowLiveAdjustZ

WindowLiveAdjustZ::WindowLiveAdjustZ(window_t *parent, point_i16_t pt)
    : window_frame_t(parent, GuiDefaults::RectScreenBody)
    , number(this, getNumberRect(pt), marlin_vars().z_offset)
    , arrows(this, getIconPoint(pt)) {

    SetRect(number.GetRect().Union(arrows.GetRect()));
    /// using window_numb to store float value of z_offset
    /// we have to set format and bigger font
    number.set_font(GuiDefaults::FontBig);
    number.SetFormat("% .3f");
}

void WindowLiveAdjustZ::Save() {
#if HAS_SHEET_PROFILES()
    /// store new z offset value into a marlin_vars & EEPROM
    SteelSheets::SetZOffset(number.GetValue());
#else
    marlin_client::set_z_offset(number.GetValue());
#endif
}

void WindowLiveAdjustZ::Change(int dif) {
    float old = number.GetValue();
    float z_offset = number.GetValue();

    const float z_offset_step = 1.0f / get_steps_per_unit_z();
    z_offset += (float)dif * z_offset_step;
    z_offset = dif >= 0 ? std::max(z_offset, old) : std::min(z_offset, old); // check overflow/underflow
    z_offset = std::min(z_offset, z_offset_max);
    z_offset = std::max(z_offset, z_offset_min);

    /// check if value has changed so we are free to set babystep and store new value
    float baby_step = z_offset - old;
    if (baby_step != 0.0F) {
        number.SetValue(z_offset);
        marlin_client::do_babysteps_Z(baby_step);
    }
}

void WindowLiveAdjustZ::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::KNOB: {
        const auto old_value = number.GetValue();

        auto &ctx = *static_cast<GuiEventContext *>(param);
        auto &ev = ctx.event.value<gui_event::KnobEvent>();

        Change(ev.diff);
        arrows.SetState(ev.diff > 0 ? WindowArrows::State_t::up : WindowArrows::State_t::down);
        sound::play(number.GetValue() != old_value ? SoundType::encoder_move : SoundType::blind_alert);

        ctx.accept();
        break;
    }

    default:
        window_frame_t::windowEvent(sender, event, param);
        break;
    }
}

/*****************************************************************************/
// WindowLiveAdjustZ_withText

WindowLiveAdjustZ_withText::WindowLiveAdjustZ_withText(window_t *parent, point_i16_t pt, size_t width)
    : WindowLiveAdjustZ(parent, pt)
    , text(parent, Rect16(), is_multiline::no, is_closed_on_click_t::no, _(text_str)) {
    Shift(ShiftDir_t::Right, width - Width());
    text.SetRect(Rect16(pt, width - Width(), Height()));
    SetRect(GetRect().Union(text.GetRect()));
}

void WindowLiveAdjustZ_withText::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::KNOB:
        if (!active) {
            return; // discard event
        }
        break;

    default:
        break;
    }
    WindowLiveAdjustZ::windowEvent(sender, event, param);
}

/*****************************************************************************/
// LiveAdjustZ
static constexpr const padding_ui8_t textPadding = { 10, 5, 0, 0 };
static constexpr const Rect16 nozzleRect = Rect16((GuiDefaults::ScreenWidth / 2) - 24, 120, 48, 48);

#if HAS_MINI_DISPLAY() || HAS_MOCK_DISPLAY()
static constexpr const Rect16 textRect = Rect16(0, 32, GuiDefaults::ScreenWidth, 4 * 30);
static constexpr const point_i16_t adjuster_pt = point_i16_t(75, 205);
static constexpr const point_i16_t scale_pt = point_i16_t(45, 125);

#elif HAS_LARGE_DISPLAY()
static constexpr const Rect16 textRect = Rect16(20, 40, GuiDefaults::ScreenWidth - 40, 3 * 30);
static constexpr const point_i16_t adjuster_pt = point_i16_t(210, 205);
static constexpr const point_i16_t scale_pt = point_i16_t(180, 125);
#endif

ScreenLiveAdjustZ::ScreenLiveAdjustZ()
    : screen_t()
    , text(this, textRect, is_multiline::yes, is_closed_on_click_t::no)
    , nozzle_icon(this, nozzleRect, &img::nozzle_shape_48x48)
    , adjuster(this, adjuster_pt)
    , scale(this, scale_pt) {

    /// title text
    text.SetText(_("Adjust the nozzle height above the heatbed by turning the knob"));
    text.SetPadding(textPadding);

    /// set right position of the nozzle for our value
    moveNozzle();
}

void ScreenLiveAdjustZ::moveNozzle() {
    uint16_t old_top = nozzle_icon.Top();
    Rect16 moved_rect = nozzleRect; // starting position - 0%
    float relative = (z_offset_max - adjuster.GetValue()) / (z_offset_max - z_offset_min); // relative z_offset value

    // set move for a scale line indicator
    scale.SetMark(relative);

    moved_rect += Rect16::Top_t(int(40 * relative)); // how much will nozzle move
    nozzle_icon.SetRect(moved_rect);
    if (old_top != nozzle_icon.Top()) {
        nozzle_icon.Invalidate();
    }
}

void ScreenLiveAdjustZ::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::KNOB:
        adjuster.WindowEvent(sender, event, param);
        moveNozzle();
        break;

    case GUI_event_t::CLICK:
        Screens::Access()->Close();
        break;

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        sound::play(SoundType::button_echo);
        Screens::Access()->Close();
        break;

    default:
        break;
    }
}

void open_live_adjust_z_screen() {
    if (!Screens::Access()->IsScreenOpened(ScreenFactory::Screen<ScreenLiveAdjustZ>) && gui_get_nesting() <= 1) {
        Screens::Access()->Open(ScreenFactory::Screen<ScreenLiveAdjustZ>);
    }
}
