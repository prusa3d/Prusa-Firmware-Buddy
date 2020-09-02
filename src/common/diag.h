// diag.h

#pragma once

enum {
    DIAG_OK = 0,        // all OK
    DIAG_ERR_CHECKSUM,  // flash checksum error
    DIAG_ERR_I2CEEPROM, // i2c eeprom st25dv64k communication error
    DIAG_ERR_SPIFLASH,  // spi flash communication error
    DIAG_ERR_USBA,      // USB host error
    DIAG_ERR_TMC_X,     // tmc2209 X-axis communication error
    DIAG_ERR_TMC_Y,     // tmc2209 Y-axis communication error
    DIAG_ERR_TMC_Z,     // tmc2209 Z-axis communication error
    DIAG_ERR_TMC_E,     // tmc2209 E-axis communication error
    DIAG_ERR_NOLOCK,    // board not locked
    DIAG_ERR_MACADDR,   // MAC address not set or invalid
    DIAG_ERR_BOARDREV,  // board revision not set or invalid
    DIAG_ERR_TIMESTAMP, // timestamp not set or invalid
    DIAG_ERR_SERIAL,    // serial not set
};

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
