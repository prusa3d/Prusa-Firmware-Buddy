#pragma once

namespace grayscale {
/**
 * @brief Converts RGB colour to grayscale luminosity. 255 == all white, 0 == all black
 *
 * @param red
 * @param green
 * @param blue
 */
inline int to_grayscale(uint8_t red, uint8_t green, uint8_t blue) {
    return (77 * red + 150 * green + 29 * blue) >> 8;
}

/**
 * @brief Returns true if given colour is closer to white after being converted to grayscale
 *
 * @param red
 * @param green
 * @param blue
 */
inline bool is_closer_to_white(uint8_t red, uint8_t green, uint8_t blue) {
    return to_grayscale(red, green, blue) > 127;
}
} // namespace grayscale
