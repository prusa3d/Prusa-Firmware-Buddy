//----------------------------------------------------------------------------//
// hwio.h - hardware input output abstraction
#pragma once

#include <device/board.h>
#include <printers.h>
#include <inttypes.h>

// pwm outputs
enum {
    HWIO_PWM_HEATER_BED, // BED PWM
    HWIO_PWM_HEATER_0, // NOZZLE PWM
    HWIO_PWM_FAN1, // PRINT FAN?
    HWIO_PWM_FAN, // NOZZLE FAN?
#if BOARD_IS_XBUDDY()
    #if PRINTER_IS_PRUSA_iX()
    HWIO_PWM_TURBINE = HWIO_PWM_HEATER_BED
    #endif
#endif
};

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// tone
extern float hwio_beeper_get_vol(void);
extern void hwio_beeper_set_vol(float vol);
extern void hwio_beeper_set_pwm(uint32_t per, uint32_t pul);
extern void hwio_beeper_tone(float frq, uint32_t del);
extern void hwio_beeper_tone2(float frq, uint32_t del, float vol);
extern void hwio_beeper_notone(void);

// cycle 1ms
extern void hwio_update_1ms(void);

// data from loveboard eeprom
#if (BOARD_IS_XBUDDY() && HAS_TEMP_HEATBREAK)
extern uint8_t hwio_get_loveboard_bomid();
#endif

#ifdef __cplusplus
}
#endif //__cplusplus
