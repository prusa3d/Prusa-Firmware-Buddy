/*
 * filament.h
 */

#pragma once
#include <stdio.h>

typedef struct {
    const char *name;
    const char *long_name;
    uint16_t nozzle;
    uint16_t heatbed;
} filament_t;

typedef enum {
    FILAMENT_NONE = 0,
    FILAMENT_PLA,
    FILAMENT_PETG,
    FILAMENT_ASA,
    FILAMENT_ABS,
    FILAMENT_PC,
    FILAMENT_FLEX,
    FILAMENT_HIPS,
    FILAMENT_PP,
    FILAMENTS_END
} FILAMENT_t;

#define DEFAULT_FILAMENT FILAMENT_PLA

#define PREHEAT_TEMP        170.01f // like 170 but user cannot set it
#define PREHEAT_TEMP_STRING "170"   // used for G codes

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
