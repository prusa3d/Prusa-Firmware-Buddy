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

#ifdef __cplusplus
}
}; // namespace led
#endif
