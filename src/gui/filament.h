/*
 * filament.h
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#pragma once
#include <stdio.h>
#pragma pack(push)
#pragma pack(1)

typedef struct {
    const char *name;
    const char *long_name;
    uint16_t nozzle;
    uint16_t heatbed;
} filament_t;

#pragma pack(pop)

typedef enum {
    FILAMENT_NONE = 0,
    FILAMENT_PLA,
    FILAMENT_PETG,
    FILAMENT_ASA,
    FILAMENT_FLEX,
    FILAMENTS_END
} FILAMENT_t;

#define DEFAULT_FILAMENT FILAMENT_PLA
//#define FILAMENT_COUNT ((uint32_t)FILAMENTS_END-1)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//todo remove this variable after pause refactoring
extern FILAMENT_t filament_to_load;

extern const filament_t filaments[FILAMENTS_END];

void set_filament(FILAMENT_t filament);

FILAMENT_t get_filament();

FILAMENT_t get_filament_from_string(const char *s, size_t len);

#ifdef __cplusplus
}
#endif //__cplusplus
