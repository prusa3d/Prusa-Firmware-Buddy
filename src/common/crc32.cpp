#include <inttypes.h>
#include "crc32.h"
#include <config.h>
#include <string.h>
#ifdef CRC32_USE_HW
    #include "cmsis_os.h"
    #include "stm32f4xx_hal.h"
#endif

#ifdef CRC32_USE_HW
osMutexDef(crc32_hw_mutex);
osMutexId crc32_hw_mutex_id;
#endif // CRC32_USE_HW

void crc32_init(void) {
#ifdef CRC32_USE_HW
    crc32_hw_mutex_id = osMutexCreate(osMutex(crc32_hw_mutex));
    // Enable the peripheral's clock
    __HAL_RCC_CRC_CLK_ENABLE();
#endif // CRC32_USE_HW
}

#ifdef CRC32_USE_HW
static uint32_t reverse_crc32(uint32_t current_crc, uint32_t desired_crc) {
    static const uint32_t table[16] = {
        0x00000000, 0xB2B4BCB6, 0x61A864DB, 0xD31CD86D, 0xC350C9B6, 0x71E47500, 0xA2F8AD6D, 0x104C11DB,
        0x82608EDB, 0x30D4326D, 0xE3C8EA00, 0x517C56B6, 0x4130476D, 0xF384FBDB, 0x209823B6, 0x922C9F00
    };
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    desired_crc = (desired_crc >> 4) ^ table[desired_crc & 0x0F];
    return desired_crc ^ current_crc;
}

    #ifdef CRC32_USE_HW
static uint32_t crc32_hw(const uint8_t *buffer, uint32_t length, uint32_t crc) {
    if (length == 0) {
        return crc;
    }
    // ensure nobody else uses the peripheral
    osMutexWait(crc32_hw_mutex_id, osWaitForever);

    // prepare the CRC unit
    if (crc == 0) {
        // standard reset
        CRC->CR = CRC_CR_RESET;
    } else {
        // reconstruct the desired crc value within the DR register (if it's not there already)
        uint32_t desired_crc = __RBIT(crc ^ 0xFFFFFFFF);
        if (CRC->DR != desired_crc) {
            CRC->DR = reverse_crc32(CRC->DR, desired_crc);
        }
    }

    // calculate the CRC32 value
    uint32_t word_count = length;
    while (word_count--) {
        uint32_t word;
        // This is almost like
        //
        //   word = *(const uint32_t *) buffer
        //
        // But that could create mis-alligned pointer and alias something
        // that's not uint32_t in reality, and both is UB. memcpy overcomes
        // both these issues, but disappears from the actual generated code ‒
        // it simply tells the compiler it must not make some assumptions about
        // the pointer.
        memcpy(&word, buffer, 4);
        buffer += 4;
        CRC->DR = __RBIT(word);
    }
    uint32_t result = __RBIT(CRC->DR) ^ 0xFFFFFFFF;

    // release the peripheral
    osMutexRelease(crc32_hw_mutex_id);

    return result;
}

uint32_t crc32_eeprom(const uint32_t *buffer, uint32_t length) {
    // ensure nobody else uses the peripheral
    osMutexWait(crc32_hw_mutex_id, osWaitForever);
    // prepare the CRC unit
    CRC->CR = CRC_CR_RESET;
    // calculate the CRC32 value
    while (length--) {
        CRC->DR = *((uint32_t *)buffer++);
    }
    uint32_t result = CRC->DR;
    // release the peripheral
    osMutexRelease(crc32_hw_mutex_id);
    return result;
}
    #endif

#else
uint32_t crc32_eeprom(const uint32_t *buffer, uint32_t length) {
    return crc32_calc((uint8_t *)buffer, length * 4);
}
#endif

extern uint32_t crc32_sw(const uint8_t *buffer, uint32_t length, uint32_t crc) {
    uint32_t value = crc ^ 0xFFFFFFFF;
    while (length--) {
        value ^= (uint32_t)*buffer++;
        for (int bit = 0; bit < 8; bit++) {
            if (value & 1) {
                value = (value >> 1) ^ 0xEDB88320;
            } else {
                value >>= 1;
            }
        }
    }
    value ^= 0xFFFFFFFF;
    return value;
}

extern uint32_t crc32_calc_ex(uint32_t crc, const uint8_t *data, uint32_t count) {
#ifdef CRC32_USE_HW
    // use the hw peripheral to calculate crc for all full words
    uint32_t word_count = count / 4;
    crc = crc32_hw(data, word_count, crc);
    count -= word_count * 4;
    data += (word_count * 4);
#endif // CRC32_USE_HW

    // use the software implementation to calculate the rest
    crc = crc32_sw(data, count, crc);

    return crc;
}

uint32_t crc32_calc(const uint8_t *data, uint32_t count) {
    return crc32_calc_ex(0, data, count);
}
