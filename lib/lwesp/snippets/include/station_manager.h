#ifndef SNIPPET_HDR_STATION_MANAGER_H
#define SNIPPET_HDR_STATION_MANAGER_H

#include <stdint.h>
#include "lwesp/lwesp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief           Lookup table for preferred SSIDs with password for auto connect feature
 */
typedef struct {
    const char* ssid;
    const char* pass;
} ap_entry_t;

lwespr_t      connect_to_preferred_access_point(uint8_t unlimited);
void        start_access_point_scan_and_connect_procedure(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SNIPPET_HDR_STATION_MANAGER_H */
