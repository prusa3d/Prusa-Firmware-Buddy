#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t ethernetif_init(struct netif *netif);

void ethernetif_input_once(struct netif *netif);
void ethernetif_update_config(struct netif *netif);
void ethernetif_notify_conn_changed(struct netif *netif);
uint8_t *ethernetif_get_mac();
uint32_t ethernetif_link(const void *arg);

u32_t sys_jiffies(void);
u32_t sys_now(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
