//----------------------------------------------------------------------------//
// hwio.h - hardware input output abstraction
#pragma once

#include <inttypes.h>
#include <option/has_local_bed.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// tone
extern float hwio_beeper_get_vol(void);
extern void hwio_beeper_set_vol(float vol);
extern void hwio_beeper_set_pwm(uint32_t per, uint32_t pul);
extern void hwio_beeper_tone(float frq, uint32_t duration_ms);
extern void hwio_beeper_tone2(float frq, uint32_t duration_ms, float vol);
extern void hwio_beeper_notone(void);

// cycle 1ms
extern void hwio_update_1ms(void);

/// Drive the nozzle heater PWM directly, 0 = off.
void hwio_nozzle_heater_write_pwm(uint8_t pwm);

#if HAS_LOCAL_BED()
void analogWrite_HEATER_BED(uint32_t);
uint32_t analogRead_TEMP_BED();
#endif

#ifdef __cplusplus
}
#endif //__cplusplus
