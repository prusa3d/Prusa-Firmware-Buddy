//appmain.cpp - arduino-like app start

#include "appmain.hpp"
#include "app.h"
#include "app_metrics.h"
#include "dbg.h"
#include "cmsis_os.h"
#include "config.h"
#include "dbg.h"
#include "adc.h"
#include "Jogwheel.hpp"
#include "hwio.h"
#include "sys.h"
#include "gpio.h"
#include "sound.hpp"
#include "language_eeprom.hpp"

#ifdef SIM_HEATER
    #include "sim_heater.h"
#endif //SIM_HEATER

#ifdef SIM_MOTION
    #include "sim_motion.h"
#endif //SIM_MOTION

#include "uartslave.h"
#include "marlin_server.h"
#include "bsod.h"
#include "eeprom.h"
#include "diag.h"
#include "safe_state.h"
#include "crc32.h"
#include "ff.h"
#include "dump.h"

#include <Arduino.h>
#include "trinamic.h"
#include "../Marlin/src/module/configuration_store.h"

#ifdef NEW_FANCTL
    #include "fanctl.h"
CFanCtl fanctl0 = CFanCtl(
    buddy::hw::fan0pwm,
    buddy::hw::fan0tach,
    FANCTL0_PWM_MIN, FANCTL0_PWM_MAX,
    FANCTL0_RPM_MIN, FANCTL0_RPM_MAX,
    FANCTL0_PWM_THR,
    is_autofan_t::no);
CFanCtl fanctl1 = CFanCtl(
    buddy::hw::fan1pwm,
    buddy::hw::fan1tach,
    FANCTL1_PWM_MIN, FANCTL1_PWM_MAX,
    FANCTL1_RPM_MIN, FANCTL1_RPM_MAX,
    FANCTL1_PWM_THR,
    is_autofan_t::yes);
#endif //NEW_FANCTL

#define DBG _dbg0 //debug level 0
//#define DBG(...)  //disable debug

extern void USBSerial_put_rx_data(uint8_t *buffer, uint32_t length);

extern void reset_trinamic_drivers();

extern "C" {

extern uartrxbuff_t uart6rxbuff; // PUT rx buffer
extern uartslave_t uart6slave;   // PUT slave

#ifdef BUDDY_ENABLE_ETHERNET
extern osThreadId webServerTaskHandle; // Webserver thread(used for fast boot mode)
#endif                                 //BUDDY_ENABLE_ETHERNET

void app_setup(void) {
    if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
        init_tmc();
    } else {
        init_tmc_bare_minimum();
    }

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

#ifdef BUDDY_ENABLE_ETHERNET
    if (diag_fastboot)
        osThreadResume(webServerTaskHandle);
#endif //BUDDY_ENABLE_ETHERNET

    LangEEPROM::getInstance();

    marlin_server_init();
    marlin_server_idle_cb = app_idle;

    adc_init();

#ifdef SIM_HEATER
    sim_heater_init();
#endif //SIM_HEATER

    //DBG("before setup (%ld ms)", HAL_GetTick());
    if (diag_fastboot || (!sys_fw_is_valid())) {
        if (!sys_fw_is_valid()) // following code will be done only with invalidated firmware
        {
            hwio_safe_state(); // safe states
            for (int i = 0; i < hwio_fan_get_cnt(); ++i)
                hwio_fan_set_pwm(i, 0); // disable fans
        }
        reset_trinamic_drivers();
        if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
            init_tmc();
        }
    } else {
        app_setup();
        marlin_server_start_processing();
    }
    //DBG("after setup (%ld ms)", HAL_GetTick());

    if (eeprom_get_init_status() == EEPROM_INIT_Defaults && marlin_server_processing()) {
        settings.reset();
    }

    while (1) {
        if (marlin_server_processing()) {
            loop();
        }
        uartslave_cycle(&uart6slave);
        marlin_server_loop();
        osDelay(0); // switch to other threads - without this is UI slow
#ifdef JOGWHEEL_TRACE
        static int signals = jogwheel_signals;
        if (signals != jogwheel_signals) {
            signals = jogwheel_signals;
            DBG("%d %d", signals, jogwheel_encoder);
        }
#endif //JOGWHEEL_TRACE
#ifdef SIM_MOTION_TRACE_X
        static int32_t x = sim_motion_pos[0];
        if (x != sim_motion_pos[0]) {
            x = sim_motion_pos[0];
            DBG("X:%li", x);
        }
#endif //SIM_MOTION_TRACE_X
#ifdef SIM_MOTION_TRACE_Y
        static int32_t y = sim_motion_pos[1];
        if (y != sim_motion_pos[1]) {
            y = sim_motion_pos[1];
            DBG("Y:%li", y);
        }
#endif //SIM_MOTION_TRACE_Y
#ifdef SIM_MOTION_TRACE_Z
        static int32_t z = sim_motion_pos[2];
        if (z != sim_motion_pos[2]) {
            z = sim_motion_pos[2];
            DBG("Z:%li", z);
        }
#endif //SIM_MOTION_TRACE_Z
#if defined(FANCTL0_TRACE) && defined(FANCTL0_TRACE)
        static uint16_t rpm0_tmp = 0;
        static uint16_t rpm1_tmp = 0;
        uint16_t rpm0 = fanctl0.getActualRPM();
        uint16_t rpm1 = fanctl1.getActualRPM();
        if ((rpm0_tmp != rpm0) || (rpm1_tmp != rpm1)) {
            rpm0_tmp = rpm0;
            rpm1_tmp = rpm1;
            _dbg("rpm0: %-5u rpm1: %-5u", rpm0, rpm1);
        }
#else //defined(FANCTL0_TRACE) && defined(FANCTL0_TRACE)
    #ifdef FANCTL0_TRACE
        static uint16_t rpm0_tmp = 0;
        uint16_t rpm0 = fanctl0.getActualRPM();
        if (rpm0_tmp != rpm0) {
            rpm0_tmp = rpm0;
            _dbg("rpm0: %u", rpm0);
        }
    #endif //FANCTL0_TRACE
    #ifdef FANCTL1_TRACE
        static uint16_t rpm1_tmp = 0;
        uint16_t rpm1 = fanctl1.getActualRPM();
        if (rpm1_tmp != rpm1) {
            rpm1_tmp = rpm1;
            _dbg("rpm1: %u", rpm1);
        }
    #endif //FANCTL1_TRACE
#endif     //defined(FANCTL0_TRACE) && defined(FANCTL0_TRACE)
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
    adc_cycle();
#ifdef SIM_HEATER
    static uint8_t cnt_sim_heater = 0;
    if (++cnt_sim_heater >= 50) // sim_heater freq = 20Hz
    {
        sim_heater_cycle();
        cnt_sim_heater = 0;
    }
#endif //SIM_HEATER

#ifdef SIM_MOTION
    sim_motion_cycle();
#endif //SIM_MOTION
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
