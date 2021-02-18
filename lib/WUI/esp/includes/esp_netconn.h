#include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         LWESP_API
 * \defgroup        LWESP_NETCONN Network connection
 * \brief           Network connection
 * \{
 */

struct esp_netconn;

/**
 * \brief           Netconn object structure
 */
typedef struct esp_netconn *esp_netconn_p;

/**
 * \brief           Receive data with no timeout
 * \note            Used with \ref esp_netconn_set_receive_timeout function
 */
#define LWESP_NETCONN_RECEIVE_NO_WAIT 0xFFFFFFFF

/**
 * \brief           Netconn connection type
 */
typedef enum {
    LWESP_NETCONN_TYPE_TCP = LWESP_CONN_TYPE_TCP, /*!< TCP connection */
    LWESP_NETCONN_TYPE_SSL = LWESP_CONN_TYPE_SSL, /*!< SSL connection */
    LWESP_NETCONN_TYPE_UDP = LWESP_CONN_TYPE_UDP, /*!< UDP connection */
} esp_netconn_type_t;

esp_netconn_p esp_netconn_new(esp_netconn_type_t type);
espr_t esp_netconn_delete(esp_netconn_p nc);
espr_t esp_netconn_bind(esp_netconn_p nc, esp_port_t port);
espr_t esp_netconn_connect(esp_netconn_p nc, const char *host, esp_port_t port);
espr_t esp_netconn_receive(esp_netconn_p nc, esp_pbuf_p *pbuf);
espr_t esp_netconn_close(esp_netconn_p nc);
int8_t esp_netconn_get_connnum(esp_netconn_p nc);
esp_conn_p esp_netconn_get_conn(esp_netconn_p nc);
void esp_netconn_set_receive_timeout(esp_netconn_p nc, uint32_t timeout);
uint32_t esp_netconn_get_receive_timeout(esp_netconn_p nc);

espr_t esp_netconn_connect_ex(esp_netconn_p nc, const char *host, esp_port_t port,
    uint16_t keep_alive, const char *local_ip, esp_port_t local_port, uint8_t mode);

/* TCP only */
espr_t esp_netconn_listen(esp_netconn_p nc);
espr_t esp_netconn_listen_with_max_conn(esp_netconn_p nc, uint16_t max_connections);
espr_t esp_netconn_set_listen_conn_timeout(esp_netconn_p nc, uint16_t timeout);
espr_t esp_netconn_accept(esp_netconn_p nc, esp_netconn_p *client);
espr_t esp_netconn_write(esp_netconn_p nc, const void *data, size_t btw);
espr_t esp_netconn_flush(esp_netconn_p nc);

/* UDP only */
espr_t esp_netconn_send(esp_netconn_p nc, const void *data, size_t btw);
espr_t esp_netconn_sendto(esp_netconn_p nc, const esp_ip_t *ip, esp_port_t port, const void *data, size_t btw);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
