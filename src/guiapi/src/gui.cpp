// gui.cpp
#include <stdlib.h>

#include "display.h"
#include "gui.hpp"
#include "gui_time.hpp" //gui::GetTick
#include "ScreenHandler.hpp"
#include "window_dlg_strong_warning.hpp"
#include "IDialog.hpp"
#include "Jogwheel.hpp"
#include "ScreenShot.hpp"
#include "gui_media_events.hpp"
#include "gui_invalidate.hpp"
#include "knob_event.hpp"
#include "touch_get.hpp"
#include "touch_dependency.hpp"
#include "marlin_client.hpp"
#include "sw_timer.hpp"
#include "log.h"

LOG_COMPONENT_REF(GUI);
LOG_COMPONENT_REF(Touch);

static const constexpr uint16_t GUI_FLG_INVALID = 0x0001;

static bool gui_invalid = false;

// touch driver must redefine this function to work
// but gui does not need single preprocessor macro to handle not having touch driver
std::optional<point_ui16_t> __attribute__((weak)) touch::Get() {
    //    log_error(GUI, "%s needs to be defined for touch to work", __PRETTY_FUNCTION__);
    return std::nullopt;
}
bool __attribute__((weak)) touch::is_hw_broken() {
    //    log_error(GUI, "%s needs to be defined for touch to work", __PRETTY_FUNCTION__);
    return true;
}

bool __attribute__((weak)) touch::does_read_work() {
    //    log_error(GUI, "%s needs to be defined for touch to work", __PRETTY_FUNCTION__);
    return false;
}

// shadow touch weak functions, so touch driver is not dependent on eeprom
bool touch::is_enabled() {
    return eeprom_get_bool(EEVAR_TOUCH_ENABLED);
}

void touch::enable() {
    eeprom_set_bool(EEVAR_TOUCH_ENABLED, true);
    log_info(Touch, "enabled");
}

void touch::disable() {
    eeprom_set_bool(EEVAR_TOUCH_ENABLED, false);
    log_info(Touch, "disabled");
}

#ifdef GUI_USE_RTOS
osThreadId gui_task_handle = 0;
#endif // GUI_USE_RTOS

font_t *GuiDefaults::Font = nullptr;
font_t *GuiDefaults::FontBig = nullptr;
font_t *GuiDefaults::FontMenuItems = nullptr;
font_t *GuiDefaults::FontMenuSpecial = nullptr;
font_t *GuiDefaults::FooterFont = nullptr;

constexpr padding_ui8_t GuiDefaults::Padding;
constexpr Rect16 GuiDefaults::RectHeader;
constexpr Rect16 GuiDefaults::RectScreenBody;
constexpr Rect16 GuiDefaults::RectScreen;
constexpr Rect16 GuiDefaults::RectScreenNoFoot;
constexpr Rect16 GuiDefaults::RectScreenNoHeader;
constexpr Rect16 GuiDefaults::RectFooter;

static const constexpr uint32_t GUI_DELAY_MIN = 1;
static const constexpr uint32_t GUI_DELAY_MAX = 10;
static const constexpr uint8_t GUI_DELAY_LOOP = 100;
static const constexpr uint32_t GUI_DELAY_REDRAW = 40; // 40 ms => 25 fps

static Sw_Timer<uint32_t> gui_loop_timer(GUI_DELAY_LOOP);
static Sw_Timer<uint32_t> gui_redraw_timer(GUI_DELAY_REDRAW);

void gui_init(void) {
    display::Init();
    gui_task_handle = osThreadGetId();

    // select jogwheel type by measured 'reset delay'
    // original displays with 15 position encoder returns values 1-2 (short delay - no capacitor)
    // new displays with MK3 encoder returns values around 16000 (long delay - 100nF capacitor)
#ifdef GUI_JOGWHEEL_SUPPORT
    #ifdef USE_ST7789
    // run-time jogwheel type detection decides which type of jogwheel device has (each type has different encoder behaviour)
    jogwheel.SetJogwheelType(st7789v_reset_delay);
    #else /* ! USE_ST7789 */
    jogwheel.SetJogwheelType(0);
    #endif
#endif

    GuiDefaults::Font = resource_font(IDR_FNT_NORMAL);
    GuiDefaults::FontBig = resource_font(IDR_FNT_BIG);
    GuiDefaults::FontMenuItems = resource_font(IDR_FNT_NORMAL);
    GuiDefaults::FontMenuSpecial = resource_font(IDR_FNT_SPECIAL);
    GuiDefaults::FooterFont = resource_font(IDR_FNT_SPECIAL);
}

#ifdef GUI_JOGWHEEL_SUPPORT
void gui_handle_jogwheel() {
    BtnState_t btn_ev;
    bool is_btn = jogwheel.ConsumeButtonEvent(btn_ev);
    int32_t encoder_diff = jogwheel.ConsumeEncoderDiff();

    if (encoder_diff != 0 || is_btn) {
        gui::knob::EventEncoder(encoder_diff);

        if (is_btn) {
            gui::knob::EventClick(btn_ev);
        }
    }
}
#endif // GUI_JOGWHEEL_SUPPORT

void gui_redraw(void) {
    uint32_t now = ticks_ms();
    bool should_sleep = true;
    if (gui_invalid) {
        if (gui_redraw_timer.RestartIfIsOver(now)) {
            Screens::Access()->Draw();
            gui_invalid = false;
            should_sleep = false;
        }
    }

    if (should_sleep) {
        uint32_t sleep = std::clamp(gui_redraw_timer.Remains(now), GUI_DELAY_MIN, GUI_DELAY_MAX);
        osDelay(sleep);
    }
}

// at least one window is invalid
void gui_invalidate(void) {
    gui_invalid = true;
}

#ifdef GUI_WINDOW_SUPPORT

static uint8_t guiloop_nesting = 0;
uint8_t gui_get_nesting(void) { return guiloop_nesting; }

void gui_loop_cb() {
    marlin_client_loop();
    GuiMediaEventsHandler::Tick();
}

void gui_loop_display_warning_check() {
    window_dlg_strong_warning_t::ScreenJumpCheck();
}

void gui_bare_loop() {
    ++guiloop_nesting;

    #ifdef GUI_JOGWHEEL_SUPPORT
    gui_handle_jogwheel();
    #endif // GUI_JOGWHEEL_SUPPORT

    gui_timers_cycle();
    gui_redraw();

    if (gui_loop_timer.RestartIfIsOver(gui::GetTick())) {
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::LOOP, 0);
    }

    --guiloop_nesting;
}

void gui_loop(void) {
    ++guiloop_nesting;

    #ifdef GUI_JOGWHEEL_SUPPORT
    gui_handle_jogwheel();
    #endif // GUI_JOGWHEEL_SUPPORT

    if (HAS_TOUCH && touch::is_enabled())
        touch::poll();

    auto point = touch::Get();
    if (point) {
        event_conversion_union un;

        un.point = *point;

        window_t *capture_ptr = Screens::Access()->Get()->GetCapturedWindow();

        if (capture_ptr) {
            // we clicked on something, does not really matter on what we clicked
            // we must notify serve to so it knows user is doing something and resets menu timeout, heater timeout ...
            Screens::Access()->ResetTimeout();
            marlin_notify_server_about_knob_click();
            capture_ptr->WindowEvent(capture_ptr, GUI_event_t::TOUCH, un.pvoid);
        }
    }

    MediaState_t media_state = MediaState_t::unknown;
    if (GuiMediaEventsHandler::ConsumeSent(media_state)) {
        switch (media_state) {
        case MediaState_t::inserted:
        case MediaState_t::removed:
        case MediaState_t::error:
            Screens::Access()->ScreenEvent(nullptr, GUI_event_t::MEDIA, (void *)int(media_state));
            break;
        default:
            break;
        }
    }

    gui_timers_cycle();
    gui_redraw();
    gui_loop_cb();
    gui_loop_display_warning_check();
    if (gui_loop_timer.RestartIfIsOver(gui::GetTick())) {
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::LOOP, 0);
    }
    --guiloop_nesting;
}

#endif // GUI_WINDOW_SUPPORT
