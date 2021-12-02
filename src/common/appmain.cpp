//appmain.cpp - arduino-like app start

#include "appmain.hpp"
#include "app.h"
#include "app_metrics.h"
#include "dbg.h"
#include "cmsis_os.h"
#include "config.h"
#include "dbg.h"
#include "adc.hpp"
#include "Jogwheel.hpp"
#include "hwio.h"
#include "sys.h"
#include "gpio.h"
#include "sound.hpp"
#include "language_eeprom.hpp"
#include "usbd_cdc_if.h"

#ifdef SIM_HEATER
    #include "sim_heater.h"
#endif //SIM_HEATER

#include "marlin_server.h"
#include "bsod.h"
#include "eeprom.h"
#include "safe_state.h"
#include "crc32.h"
#include "dump.h"

#include <Arduino.h>
#include "trinamic.h"
#include "../Marlin/src/module/configuration_store.h"

#ifdef NEW_FANCTL
    #include "fanctl.h"
CFanCtl fanCtlPrint = CFanCtl(
    buddy::hw::fanPrintPwm,
    buddy::hw::fanPrintTach,
    FANCTLPRINT_PWM_MIN, FANCTLPRINT_PWM_MAX,
    FANCTLPRINT_RPM_MIN, FANCTLPRINT_RPM_MAX,
    FANCTLPRINT_PWM_THR,
    is_autofan_t::no);
CFanCtl fanCtlHeatBreak = CFanCtl(
    buddy::hw::fanHeatBreakPwm,
    buddy::hw::fanHeatBreakTach,
    FANCTLHEATBREAK_PWM_MIN, FANCTLHEATBREAK_PWM_MAX,
    FANCTLHEATBREAK_RPM_MIN, FANCTLHEATBREAK_RPM_MAX,
    FANCTLHEATBREAK_PWM_THR,
    is_autofan_t::yes);
#endif //NEW_FANCTL

#define DBG _dbg0 //debug level 0
//#define DBG(...)  //disable debug

extern void USBSerial_put_rx_data(uint8_t *buffer, uint32_t length);
extern void app_cdc_rx(uint8_t *buffer, uint32_t length);

extern void reset_trinamic_drivers();

extern "C" {

#ifndef USE_ESP01_WITH_UART6
extern uartslave_t uart6slave; // PUT slave
#endif

#ifdef BUDDY_ENABLE_ETHERNET
extern osThreadId webServerTaskHandle; // Webserver thread(used for fast boot mode)
#endif                                 //BUDDY_ENABLE_ETHERNET

void app_setup(void) {
    if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
        init_tmc();
    } else {
        init_tmc_bare_minimum();
    }

    // enable cdc
    usbd_cdc_register_receive_fn(app_cdc_rx);

    setup();

    marlin_server_settings_load(); // load marlin variables from eeprom
    //DBG("after init_tmc (%ld ms)", HAL_GetTick());
}

void app_idle(void) {
    Buddy::Metrics::RecordMarlinVariables();
    Buddy::Metrics::RecordRuntimeStats();
    Buddy::Metrics::RecordPrintFilename();
    osDelay(0); // switch to other threads - without this is UI slow during printing
}

void app_run(void) {
    DBG("app_run");

    LangEEPROM::getInstance();

    marlin_server_init();
    marlin_server_idle_cb = app_idle;

#ifdef SIM_HEATER
    sim_heater_init();
#endif //SIM_HEATER

    //DBG("before setup (%ld ms)", HAL_GetTick());

    app_setup();
    marlin_server_start_processing();

    //DBG("after setup (%ld ms)", HAL_GetTick());

    if (eeprom_init() == EEPROM_INIT_Defaults && marlin_server_processing()) {
        settings.reset();
    }

    while (1) {
        if (marlin_server_processing()) {
            loop();
        }
#ifndef USE_ESP01_WITH_UART6
        uartslave_cycle(&uart6slave);
#endif
        marlin_server_loop();
        osDelay(0); // switch to other threads - without this is UI slow
#ifdef JOGWHEEL_TRACE
        static int signals = jogwheel_signals;
        if (signals != jogwheel_signals) {
            signals = jogwheel_signals;
            DBG("%d %d", signals, jogwheel_encoder);
        }
#endif //JOGWHEEL_TRACE

#if defined(FANCTLPRINT_TRACE) && defined(FANCTLPRINT_TRACE)
        static uint16_t rpm0_tmp = 0;
        static uint16_t rpm1_tmp = 0;
        uint16_t rpm0 = fanCtlPrint.getActualRPM();
        uint16_t rpm1 = fanCtlHeatBreak.getActualRPM();
        if ((rpm0_tmp != rpm0) || (rpm1_tmp != rpm1)) {
            rpm0_tmp = rpm0;
            rpm1_tmp = rpm1;
            _dbg("rpm0: %-5u rpm1: %-5u", rpm0, rpm1);
        }
#else //defined(FANCTLPRINT_TRACE) && defined(FANCTLPRINT_TRACE)
    #ifdef FANCTLPRINT_TRACE
        static uint16_t rpm0_tmp = 0;
        uint16_t rpm0 = fanCtlPrint.getActualRPM();
        if (rpm0_tmp != rpm0) {
            rpm0_tmp = rpm0;
            _dbg("rpm0: %u", rpm0);
        }
    #endif //FANCTLPRINT_TRACE
    #ifdef FANCTLHEATBREAK_TRACE
        static uint16_t rpm1_tmp = 0;
        uint16_t rpm1 = fanCtlHeatBreak.getActualRPM();
        if (rpm1_tmp != rpm1) {
            rpm1_tmp = rpm1;
            _dbg("rpm1: %u", rpm1);
        }
    #endif //FANCTLHEATBREAK_TRACE
#endif     //defined(FANCTLPRINT_TRACE) && defined(FANCTLPRINT_TRACE)
    }
}

void app_error(void) {
    bsod("app_error");
}

void app_assert(uint8_t *file, uint32_t line) {
    bsod("app_assert");
}

void app_cdc_rx(uint8_t *buffer, uint32_t length) {
    if (!marlin_server_get_exclusive_mode()) // serial line is disabled in exclusive mode
        USBSerial_put_rx_data(buffer, length);
}

void adc_tick_1ms(void) {
#ifdef SIM_HEATER
    static uint8_t cnt_sim_heater = 0;
    if (++cnt_sim_heater >= 50) // sim_heater freq = 20Hz
    {
        sim_heater_cycle();
        cnt_sim_heater = 0;
    }
#endif //SIM_HEATER
}

void app_tim14_tick(void) {
#ifdef NEW_FANCTL
    fanctl_tick();
#endif //NEW_FANCTL
#ifndef HAS_GUI
    #error "HAS_GUI not defined."
#elif HAS_GUI
    jogwheel.Update1msFromISR();
#endif
    Sound_Update1ms();
    //hwio_update_1ms();
    adc_tick_1ms();
}

} // extern "C"

//cpp code
