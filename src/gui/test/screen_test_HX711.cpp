// screen_test_hx711.c

#include "config.h"
#if 0
    #include <stdio.h>
    #include <stdlib.h>
    #include "gui.hpp"
    #include "loadcell.hpp"
    #include "gpio.h"
    #include "screens.h"
    #include <config_store/store_instance.hpp>

typedef struct
{
    window_frame_t frame;
    window_text_t text_terminal;
    window_text_t text_scale;
    window_spin_t spin_scale;
    window_text_t text_thrs_static;
    window_spin_t spin_thrs_static;
    window_text_t text_thrs_continuous;
    window_spin_t spin_thrs_continuous;
    window_text_t text_hyst;
    window_spin_t spin_hyst;
    window_text_t text_out;
    window_text_t button_save;
    window_text_t button_return;
    char str_out[32];
    char str_term[32];
    #ifdef FILAMENT_SENSOR_ADC
    window_text_t text_terminal2;
    char str_term2[32];
    #endif
} screen_test_hx711_data_t;

enum class Action : int {
    Tare = 1,
    Save,
    Close,
    UpdateThresholdStatic,
    UpdatethresholdContinuous,
    UpdateScale,
    UpdateHysteresis,
};

    #define pd ((screen_test_hx711_data_t *)screen->pdata)

void screen_test_hx711_init(screen_t *screen) {
    uint16_t y;
    uint16_t x;
    uint16_t w;
    uint16_t h;

    //font_t* _font_big = resource_font(IDR_FNT_BIG);
    font_t *_font_term = resource_font(IDR_FNT_SMALL);

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    y = 0;
    x = 10;
    w = 220;
    h = 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_terminal));
    pd->text_terminal.set_font(_font_term);

    y += 22;
    #ifdef FILAMENT_SENSOR_ADC
    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_terminal2));
    pd->text_terminal2.set_font(_font_term);
    #endif
    y += 22;

    w = 140;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_scale));
    pd->text_scale.set_font(_font_term);
    static const char t_scale[] = "scale []";
    pd->text_scale.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)t_scale));

    y += 22;

    window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_scale));
    pd->spin_scale.SetFormat("%.1f");
    pd->spin_scale.SetMinMaxStep(2.0F, 30.0F, 0.10F);
    pd->spin_scale.SetValue(loadcell.GetScale() * 1000);
    pd->spin_scale.SetTag((int)Action::UpdateScale);

    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_thrs_static));
    pd->text_thrs_static.set_font(_font_term);
    static const char t_thrs[] = "threshold (MBL) [g]";
    pd->text_thrs_static.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)t_thrs));

    y += 22;

    window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_thrs_static));
    pd->spin_thrs_static.SetFormat("%.0f");
    pd->spin_thrs_static.SetMinMaxStep(-500.0F, -5.0F, 5.0F);
    pd->spin_thrs_static.SetValue(loadcell.GetThreshold(Loadcell::TareMode::Static));
    pd->spin_thrs_static.SetTag((int)Action::UpdateThresholdStatic);

    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_thrs_continuous));
    pd->text_thrs_continuous.set_font(_font_term);
    static const char t_thrs_continuous[] = "threshold (home) [g]";
    pd->text_thrs_continuous.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)t_thrs_continuous));

    y += 22;

    window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_thrs_continuous));
    pd->spin_thrs_continuous.SetFormat("%.0f");
    pd->spin_thrs_continuous.SetMinMaxStep(-500.0F, -5.0F, 5.0F);
    pd->spin_thrs_continuous.SetValue(loadcell.GetThreshold(Loadcell::TareMode::Continuous));
    pd->spin_thrs_continuous.SetTag((int)Action::UpdatethresholdContinuous);
    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_hyst));
    pd->text_hyst.set_font(_font_term);
    static const char t_hyst[] = "hysteresis [g]";
    pd->text_hyst.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)t_hyst));

    y += 22;

    window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_hyst));
    pd->spin_hyst.SetFormat("%.0f");
    pd->spin_hyst.SetMinMaxStep(0.0F, 100.0F, 5.0F);
    pd->spin_hyst.SetValue(loadcell.GetHysteresis());
    pd->spin_hyst.SetTag((int)Action::UpdateHysteresis);

    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_out));
    pd->text_out.set_font(_font_term);

    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->button_save));
    static const char t_save[] = "Save";
    pd->button_save.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)t_save));
    pd->button_save.Enable();
    pd->button_save.SetTag((int)Action::Save);

    y += 22;

    window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->button_return));
    static const char t_return[] = "Return";
    pd->button_return.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)t_return));
    pd->button_return.Enable();
    pd->button_return.SetTag((int)Action::Close);
}

void screen_test_hx711_done(screen_t *screen) {
    window_destroy(pd->frame.id);
}

void screen_test_hx711_draw(screen_t *screen) {
}

int screen_test_hx711_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK) {
        switch (Action((int)param)) {
        case Action::Save: // save
            config_store().loadcell_scale.set(loadcell.GetScale());
            config_store().loadcell_threshold_static.set(loadcell.GetThreshold(Loadcell::TareMode::Static));
            config_store().loadcell_threshold_continuous.set(loadcell.GetThreshold(Loadcell::TareMode::Continuous));
            config_store().loadcell_hysteresis.set(loadcell.GetHysteresis());
            break;
        case Action::Close: // return
            screen_close();
            return 1;
        default:
            break;
        }
    } else if (event == WINDOW_EVENT_CHANGE) {
        switch (Action((int)param)) {
        case Action::UpdateScale:
            loadcell.SetScale(pd->spin_scale.GetValue() / 1000.0F);
            break;
        case Action::UpdateThresholdStatic:
            loadcell.SetThreshold(pd->spin_thrs_static.GetValue(), Loadcell::TareMode::Static);
            break;
        case Action::UpdatethresholdContinuous:
            loadcell.SetThreshold(pd->spin_thrs_continuous.GetValue(), Loadcell::TareMode::Continuous);
            break;
        case Action::UpdateHysteresis:
            loadcell.SetHysteresis(pd->spin_hyst.GetValue());
            break;
        default:
            break;
        }
    } else if (event == WINDOW_EVENT_LOOP) {
        sprintf(pd->str_out, "%ld", loadcell.GetRawValue());
        pd->text_out.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)pd->str_out));
        sprintf(pd->str_term, "LC: %d, RAW: %.1f", loadcell.GetMinZEndstop(), (double)loadcell.GetLoad());
        pd->text_terminal.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)pd->str_term));
    #ifdef FILAMENT_SENSOR_ADC
        //sprintf(pd->str_term2, "FS: %d, RAW: %ld", fsensor_probe, fsensor_value);
        pd->text_terminal2.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)pd->str_term2));
    #endif
    }
    return 0;
}

screen_t screen_test_hx711 = {
    0,
    0,
    screen_test_hx711_init,
    screen_test_hx711_done,
    screen_test_hx711_draw,
    screen_test_hx711_event,
    sizeof(screen_test_hx711_data_t), //data_size
    0,                                //pdata
};

screen_t *const get_scr_test_hx711() { return &screen_test_hx711; }
#endif // 0
