#include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         LWESP
 * \defgroup        LWESP_CONN Connection API
 * \brief           Connection API functions
 * \{
 */

esp_res_t esp_conn_start(esp_conn_p *conn, esp_conn_type_t type, const char *const remote_host, esp_port_t remote_port, void *const arg, esp_evt_fn conn_evt_fn, const uint32_t blocking);
esp_res_t esp_conn_startex(esp_conn_p *conn, esp_conn_start_t *start_struct, void *const arg, esp_evt_fn conn_evt_fn, const uint32_t blocking);

esp_res_t esp_conn_close(esp_conn_p conn, const uint32_t blocking);
esp_res_t esp_conn_send(esp_conn_p conn, const void *data, size_t btw, size_t *const bw, const uint32_t blocking);
esp_res_t esp_conn_sendto(esp_conn_p conn, const esp_ip_t *const ip, esp_port_t port, const void *data, size_t btw, size_t *bw, const uint32_t blocking);
esp_res_t esp_conn_set_arg(esp_conn_p conn, void *const arg);
void *esp_conn_get_arg(esp_conn_p conn);
uint8_t esp_conn_is_client(esp_conn_p conn);
uint8_t esp_conn_is_server(esp_conn_p conn);
uint8_t esp_conn_is_active(esp_conn_p conn);
uint8_t esp_conn_is_closed(esp_conn_p conn);
int8_t esp_conn_getnum(esp_conn_p conn);
esp_res_t esp_conn_set_ssl_buffersize(size_t size, const uint32_t blocking);
esp_res_t esp_get_conns_status(const uint32_t blocking);
esp_conn_p esp_conn_get_from_evt(esp_evt_t *evt);
esp_res_t esp_conn_write(esp_conn_p conn, const void *data, size_t btw, uint8_t flush, size_t *const mem_available);
esp_res_t esp_conn_recved(esp_conn_p conn, esp_pbuf_p pbuf);
size_t esp_conn_get_total_recved_count(esp_conn_p conn);

uint8_t esp_conn_get_remote_ip(esp_conn_p conn, esp_ip_t *ip);
esp_port_t esp_conn_get_remote_port(esp_conn_p conn);
esp_port_t esp_conn_get_local_port(esp_conn_p conn);
esp_res_t esp_conn_ssl_set_config(uint8_t link_id, uint8_t auth_mode, uint8_t pki_number, uint8_t ca_number, const esp_api_cmd_evt_fn evt_fn, void *const evt_arg, const uint32_t blocking);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
