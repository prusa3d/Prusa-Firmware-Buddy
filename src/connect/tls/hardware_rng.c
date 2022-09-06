#include <main.h>
#include <string.h>
#include <stm32f4xx_hal.h>
#include <mbedtls/entropy_poll.h>

int mbedtls_hardware_poll(void *Data, unsigned char *Output, size_t Len, size_t *oLen) {

    uint32_t randomValue;
    uint32_t bytesWritten = 0;
    *oLen = 0;

    while (bytesWritten < Len) {

        if (HAL_RNG_GenerateRandomNumber(&hrng, &randomValue) != HAL_OK) {
            return 1;
        }

        size_t bytes_left = Len - bytesWritten;
        size_t to_write = (bytes_left > sizeof randomValue) ? sizeof randomValue : bytes_left;
        memcpy((Output + bytesWritten), (const char *)&randomValue, to_write);
        bytesWritten += to_write;
    }

    *oLen = bytesWritten;
    return 0;
}
