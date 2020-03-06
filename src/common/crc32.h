// crc32.h

#ifndef _CRC32_H
#define _CRC32_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// initialize hw crc32 generator and create semaphore
extern void crc32_init(void);

// calculate crc32 for uint32_t data
extern uint32_t crc32_calc(uint32_t *data, uint32_t count);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_CRC32_H
