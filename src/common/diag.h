// diag.h

#ifndef _DIAG_H
#define _DIAG_H

#define DIAG_OK            0 // all OK
#define DIAG_ERR_CHECKSUM  1 // flash checksum error
#define DIAG_ERR_I2CEEPROM 2 // i2c eeprom st25dv64k communication error
#define DIAG_ERR_SPIFLASH  3 // spi flash communication error
#define DIAG_ERR_USBA      4 // USB host error
#define DIAG_ERR_TMC_X     5 // tmc2209 X-axis communication error
#define DIAG_ERR_TMC_Y     6 // tmc2209 Y-axis communication error
#define DIAG_ERR_TMC_Z     7 // tmc2209 Z-axis communication error
#define DIAG_ERR_TMC_E     8 // tmc2209 E-axis communication error

#define DIAG_ERR_NOLOCK    10 // board not locked
#define DIAG_ERR_MACADDR   11 // MAC address not set or invalid
#define DIAG_ERR_BOARDREV  12 // board revision not set or invalid
#define DIAG_ERR_TIMESTAMP 13 // timestamp not set or invalid
#define DIAG_ERR_SERIAL    14 // serial not set

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int diag_fastboot;

extern int diag_error;

extern void diag_check_fastboot(void);

extern void diag_test(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DIAG_H
