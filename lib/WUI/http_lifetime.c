#include "http_lifetime.h"
#include "http/httpd.h"
#include "http_handler_default.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <stdlib.h>

/**
 * @ingroup httpd
 * Current ALTCP PCB used by httpd for accepting incomming connections.
 */
static struct altcp_pcb *httpd_pcb = NULL;
// Protection of the httpd_pcb
static SemaphoreHandle_t httpd_pcb_mutex = NULL;

/**
 * @ingroup httpd
 * Initialize the httpd: set up a listening PCB and bind it to the defined port
 */
void httpd_init(void) {
    LWIP_ASSERT("httpd_init called multiple times", httpd_pcb_mutex == NULL);
    httpd_pcb_mutex = xSemaphoreCreateMutex();
    LWIP_ASSERT("Couldn't create mutex to protect http listening socket", httpd_pcb_mutex != NULL);

    httpd_init_pools();

    httpd_reinit();
}

// The inner part of httpd_close. Assumes the mutex is already locked.
static void httpd_close_locked(void) {
    // According to docs, the close can fail in case there's not enough memory.
    // That's likely because usual sockets still wait for some more data to
    // arrive (?). Hopefully this is not the case for listening sockets.
    //
    // Using altcp_abort would be better in theory (as it just wipes it and
    // can't fail), but:
    // * It crashes on ethernet for no known reason.
    // * It is not currently implemented in ESP.
    err_t err = altcp_close(httpd_pcb);
    LWIP_ASSERT("Couldn't close listening socket", err == ERR_OK);
    httpd_pcb = NULL;
}

void httpd_close(void) {
    xSemaphoreTake(httpd_pcb_mutex, portMAX_DELAY);

    httpd_close_locked();

    xSemaphoreGive(httpd_pcb_mutex);
}

void httpd_reinit(void) {
    xSemaphoreTake(httpd_pcb_mutex, portMAX_DELAY);

    if (httpd_pcb != NULL) {
        httpd_close_locked();
    }

    httpd_pcb = httpd_new(&default_http_handlers, HTTPD_SERVER_PORT);

    xSemaphoreGive(httpd_pcb_mutex);
}

#if HTTPD_ENABLE_HTTPS
/**
 * @ingroup httpd
 * Initialize the httpd: set up a listening PCB and bind it to the defined port.
 * Also set up TLS connection handling (HTTPS).
 */
void httpd_inits(struct altcp_tls_config *conf) {
    #if LWIP_ALTCP_TLS
    struct altcp_pcb *pcb_tls = altcp_tls_new(conf, IPADDR_TYPE_ANY);
    LWIP_ASSERT("httpd_init: altcp_tls_new failed", pcb_tls != NULL);
    httpd_init_pcb(pcb_tls, HTTPD_SERVER_PORT_HTTPS);
    #else  /* LWIP_ALTCP_TLS */
    LWIP_UNUSED_ARG(conf);
    #endif /* LWIP_ALTCP_TLS */
}
#endif /* HTTPD_ENABLE_HTTPS */
