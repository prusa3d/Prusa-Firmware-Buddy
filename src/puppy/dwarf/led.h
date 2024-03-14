#pragma once
#include <stdint.h>

#ifdef __cplusplus
namespace led {
extern "C" {
#endif

/**
 * @brief Sets the colour of the WS2812B-V4 LED.
 */
void set_rgb(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Sets the WS2812B-V4 LED and keeps blinking with it. This function needs to be called repeatedly to update led status
 * @param on_duration_ms MINIMUM duration the led is on
 * @param off_duration_ms MINIMUM duration the led is off
 */
void blinking(uint8_t red, uint8_t green, uint8_t blue, uint32_t on_duration_ms, uint32_t off_duration_ms);

/**
 * @brief Sets the WS2812B-V4 LED and keeps pulsing with it.
 * Pulses between color 0 and color 1.
 * This function needs to be called repeatedly to update led status.
 * @param red0 red [0 - 0xff]
 * @param green0 green [0 - 0xff]
 * @param blue0 blue [0 - 0xff]
 * @param red1 red [0 - 0xff]
 * @param green1 green [0 - 0xff]
 * @param blue1 blue [0 - 0xff]
 * @param period_ms period of the pulsing [ms]
 */
void pulsing(uint8_t red0, uint8_t green0, uint8_t blue0, uint8_t red1, uint8_t green1, uint8_t blue1, uint32_t period_ms);

#ifdef __cplusplus
}
}; // namespace led
#endif
