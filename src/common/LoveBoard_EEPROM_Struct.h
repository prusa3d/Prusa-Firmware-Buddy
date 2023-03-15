
#ifndef LOVEBOARD_EEPROM_STRUCT
#define LOVEBOARD_EEPROM_STRUCT

#include <stdint.h>

#ifdef LOVEBOARD_HAS_PT100

    #define CALIB_COUNT 3
    #define STRUCT_VER  0

struct __attribute__((packed)) calib_point {
    float resistance;
    int16_t hx_data;
};

struct __attribute__((packed)) he_therm_cal {
    uint8_t struct_ver;
    uint8_t struct_size;
    uint8_t hw_rev[3];
    uint8_t serial_number[8];
    uint8_t calib_date[3];
    uint32_t calib_station_id;
    uint8_t amplification;
    uint8_t point_count;
    uint32_t crc;
    struct calib_point calib_data[CALIB_COUNT];
};

#else // LOVEBOARD_HAS_PT100

struct __attribute__((packed)) loveboard_eeprom {
    uint8_t struct_ver;
    uint16_t struct_size;
    uint8_t bom_id;
    uint32_t timestamp;
    uint8_t datamatrix_id[24];
};

#endif // LOVEBOARD_HAS_PT100

#endif //LOVEBOARD_EEPROM_STRUCT
