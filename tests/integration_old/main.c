#include "lwip/timeouts.h"
#include "netif/tapif.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"

void lwsapi_init(void);

struct netif netif;

static void tap_netif_init(void *arg) {
    sys_sem_t *init_sem;
    LWIP_ASSERT("arg != NULL", arg != NULL);
    init_sem = (sys_sem_t *)arg;

    /* init network interfaces */
    ip4_addr_t ipaddr, netmask, gw;

    IP4_ADDR(&gw, 10, 0, 0, 1); // host IP address of TAP device
    IP4_ADDR(&ipaddr, 10, 0, 0, 2); // LwIP virtual program IP address
    IP4_ADDR(&netmask, 255, 255, 255, 0);

    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, tcpip_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

    lwsapi_init();

    sys_sem_signal(init_sem);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0); /* no stdio-buffering, please! */

    err_t err;
    sys_sem_t init_sem;

    err = sys_sem_new(&init_sem, 0);
    LWIP_ASSERT("failed to create init_sem", err == ERR_OK);
    LWIP_UNUSED_ARG(err);
    tcpip_init(tap_netif_init, &init_sem);

    /* we have to wait for initialization to finish before
     * calling update_adapter()! */
    sys_sem_wait(&init_sem);
    sys_sem_free(&init_sem);

    while (1) {
        /* poll the driver, get any outstanding frames, alloc memory for them, and
                call netif->input, which is actually ip_input() */
        sys_msleep(1);
        tapif_poll(&netif);
    }

    return 0;
}
