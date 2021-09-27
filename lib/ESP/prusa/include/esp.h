#ifndef ESP_HEADER_
#define ESP_HEADER_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief           Lookup table for preferred SSIDs with password for auto connect feature
 */
typedef struct {
    const char *ssid;
    const char *pass;
} ap_entry_t;

extern uint32_t esp_get_device_model();
extern uint32_t esp_initialize();
extern uint32_t esp_present(uint32_t);
extern uint32_t esp_is_device_presented();
extern uint32_t esp_connect_to_AP(const ap_entry_t *);

extern void esp_lwip_init(struct netif *netif);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //ESP_HEADER_
