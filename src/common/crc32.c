// crc32.c
#include "crc32.h"
#include <config.h>

#ifdef CRC32_USE_RTOS
    #include "cmsis_os.h"
// semaphore handle (lock/unlock)
osSemaphoreId crc32_sema = 0;
static inline void crc32_lock(void) {
    osSemaphoreWait(crc32_sema, osWaitForever);
}
static inline void crc32_unlock(void) {
    osSemaphoreRelease(crc32_sema);
}
#else //CRC32_USE_RTOS
    #define crc32_lock()
    #define crc32_unlock()
#endif //CRC32_USE_RTOS

#ifdef CRC32_USE_HW
    #include "tm_stm32f4_crc.h"
#else //CRC32_USE_HW

    // Polynomial used in STM32
    #define CRC32_STM_POLY 0x04C11DB7

// software calculated crc32 equal to hardware stm32 crc
uint32_t crc32_stm(uint32_t crc, uint32_t data) {
    int i;
    crc = crc ^ data;
    for (i = 0; i < 32; i++)
        if (crc & 0x80000000)
            crc = (crc << 1) ^ CRC32_STM_POLY;
        else
            crc = (crc << 1);
    return crc;
}

#endif //CRC32_USE_HW

void crc32_init(void) {
#ifdef CRC32_USE_HW
    TM_CRC_Init();
#endif //CRC32_USE_HW
#ifdef CRC32_USE_RTOS
    osSemaphoreDef(crc32Sema);
    crc32_sema = osSemaphoreCreate(osSemaphore(crc32Sema), 1);
#endif //CRC32_USE_RTOS
}

uint32_t crc32_calc(uint32_t *data, uint32_t count) {
    uint32_t crc;
    crc32_lock();
#ifdef CRC32_USE_HW
    crc = TM_CRC_Calculate32(data, count, 1);
#else  //CRC32_USE_HW
    crc = 0xFFFFFFFF;
    while (count--)
        crc = crc32_stm(crc, *(data++));
#endif //CRC32_USE_HW
    crc32_unlock();
    return crc;
}
