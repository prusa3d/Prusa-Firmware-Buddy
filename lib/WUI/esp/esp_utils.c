#include <stdint.h>
#include "esp/esp_private.h"
#include "esp/esp_utils.h"

/**
 * \brief           Convert `unsigned 32-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \param[in]       is_hex: Set to `1` to output hex, 0 otherwise
 * \param[in]       width: Width of output string.
 *                      When number is shorter than width, leading `0` characters will apply.
 *                      This parameter is valid only when formatting hex numbers
 * \return          Pointer to output variable
 */
char*
esp_u32_to_gen_str(uint32_t num, char* out, uint8_t is_hex, uint8_t width) {
    char* tmp = out;
    uint8_t i, y;

    /* Convert number to string */
    i = 0;
    tmp[0] = '0';
    if (num == 0) {
        ++i;
    } else {
        if (is_hex) {
            uint8_t mod;
            while (num > 0) {
                mod = num & 0x0F;
                if (mod < 10) {
                    tmp[i] = mod + '0';
                } else {
                    tmp[i] = mod - 10 + 'A';
                }
                num >>= 4;
                ++i;
            }
        } else {
            while (num > 0) {
                tmp[i] = (num % 10) + '0';
                num /= 10;
                ++i;
            }
        }
    }
    if (is_hex) {
        while (i < width) {
            tmp[i] = '0';
            ++i;
        }
    }
    tmp[i] = 0;

    /* Rotate string */
    y = 0;
    while (y < ((i + 1) / 2)) {
        char t = out[i - y - 1];
        out[i - y - 1] = tmp[y];
        tmp[y] = t;
        ++y;
    }
    out[i] = 0;
    return out;
}

/**
 * \brief           Convert `signed 32-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \return          Pointer to output variable
 */
char*
esp_i32_to_gen_str(int32_t num, char* out) {
    if (num < 0) {
        *out = '-';
        ++out;
        return esp_u32_to_gen_str(ESP_U32(-num), out, 0, 0) - 1;
    } else {
        return esp_u32_to_gen_str(ESP_U32(num), out, 0, 0);
    }
}
