#ifndef __STATION_MANAGER_H
#define __STATION_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp/esp.h"

/**
 * \brief           Lookup table for preferred SSIDs with password for auto connect feature
 */
typedef struct {
    const char* ssid;
    const char* pass;
} ap_entry_t;

espr_t      connect_to_preferred_access_point(uint8_t unlimited);
void        start_access_point_scan_and_connect_procedure(void);

#ifdef __cplusplus
}
#endif

#endif
