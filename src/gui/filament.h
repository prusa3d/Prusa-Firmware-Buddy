/*
 * filament.h
 *
 *  Created on: 19. 7. 2019
 *      Author: mcbig
 */

#ifndef FILAMENT_H_
#define FILAMENT_H_

#include "gui.h"

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

//#define FILAMENT_COUNT ((uint32_t)FILAMENTS_END-1)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const filament_t filaments[FILAMENTS_END];

void set_filament(FILAMENT_t filament);

FILAMENT_t get_filament();

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* FILAMENT_H_ */
