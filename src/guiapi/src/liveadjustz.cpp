// liveadjustz.cpp

#include "liveadjustz.hpp"
#include "resource.h"
#include "sound.hpp"
#include <algorithm>
#include "ScreenHandler.hpp"
#include "GuiDefaults.hpp"
#include "marlin_client.h"
#include "marlin_vars.h"
#include "eeprom.h"

#include "config.h"
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

LiveAdjustZ::LiveAdjustZ(Rect16 rect, is_closed_on_click_t outside_close)
    : IDialog(rect)
    , text(this, getTextRect(), is_multiline::yes, is_closed_on_click_t::no)
    , number(this, getNumberRect(), is_multiline::no, is_closed_on_click_t::no)
    , nozzle_icon(this, Rect16(120 - 24, 120, 48, 48), IDR_PNG_big_nozzle)
    , bed(this, Rect16(70, 180, 100, 10))
    , result(Response::_none) {

    closed_outside = outside_close;
    z_offset = marlin_vars()->z_offset;

    /// simple rectangle as bed with defined background color
    bed.SetBackColor(COLOR_ORANGE);

    /// title text
    constexpr static const char *txt = N_("Adjust the nozzle height above the heatbed by turning the knob");
    static const string_view_utf8 text_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(txt));
    text.SetText(text_view);

    /// number label
    number.font = GuiDefaults::FontBig;
    writeZoffsetLabel();
}

void LiveAdjustZ::writeZoffsetLabel(){
    snprintf(numString, 7, "%0.3f", (double)z_offset);
    static const string_view_utf8 num_view = string_view_utf8::MakeCPUFLASH((const uint8_t *)(numString));
    number.SetText(num_view);
}

Response LiveAdjustZ::GetResult() {
    return result;
}

Rect16 LiveAdjustZ::getTextRect() {
    return Rect16(0, 32, 240, 60);
}

Rect16 LiveAdjustZ::getNumberRect() {
    return Rect16(80, 205, 120, 40);
}

void LiveAdjustZ::saveAndClose() {
    variant8_t var = variant8_flt(z_offset);
    eeprom_set_var(EEVAR_ZOFFSET, var);
    marlin_set_var(MARLIN_VAR_Z_OFFSET, var);
    /// value is stored in Marlin and EEPROM, let's close the screen
    Screens::Access()->Close();
}

bool LiveAdjustZ::Change(int dif) {
    float old = z_offset;
    z_offset += (float)dif * z_offset_step;
    z_offset = dif >= 0 ? std::max(z_offset, old) : std::min(z_offset, old); //check overflow/underflow
    z_offset = std::min(z_offset, z_offset_max);
    z_offset = std::max(z_offset, z_offset_min);
    return old != z_offset;
}

void LiveAdjustZ::Step(int diff){
    auto temp_value = z_offset;
    if (Change(diff)) {
        /// wrote new value into a number label
        writeZoffsetLabel();
        /// do a baby step
        float baby_step = z_offset - temp_value;
        marlin_do_babysteps_Z(baby_step);
    }
}

void LiveAdjustZ::windowEvent(window_t *sender, uint8_t event, void *param) {
    switch (event) {
    case WINDOW_EVENT_CLICK:
        if (closed_outside == is_closed_on_click_t::yes) {
            saveAndClose();
        }
        break;
    case WINDOW_EVENT_ENC_UP:
        Step(1);
        Sound_Play(eSOUND_TYPE::EncoderMove);
        gui_invalidate();
        break;
    case WINDOW_EVENT_ENC_DN:
        Step(-1);
        Sound_Play(eSOUND_TYPE::EncoderMove);
        gui_invalidate();
        break;
    default:
        IDialog::windowEvent(sender, event, param);
    }
}

template <class T, typename... Args>
Response LiveAdjustZ_Custom(Rect16 rect, Args... args) {
    T liveadjust(rect, args...);
    liveadjust.MakeBlocking();
    return liveadjust.GetResult();
}

Response LiveAdjustZOpen(Rect16 rect, is_closed_on_click_t outside_close){
    return LiveAdjustZ_Custom<LiveAdjustZ>(rect, outside_close);
}

