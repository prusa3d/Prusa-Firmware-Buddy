#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// #include "esp/esp.h"

int32_t espi_parse_number(const char **str);
uint8_t espi_parse_string(const char **src, char *dst, size_t dst_len, uint8_t trim);
uint8_t espi_parse_ip(const char **src, esp_ip_t *ip);
uint8_t espi_parse_mac(const char **src, esp_mac_t *mac);

esp_res_t espi_parse_cipstatus(const char *str);
esp_res_t espi_parse_ipd(const char *str);
esp_res_t espi_parse_ciprecvdata(const char *str);
esp_res_t espi_parse_ciprecvlen(const char *str);

uint8_t espi_parse_cwlap(const char *str, esp_msg_t *msg);
uint8_t espi_parse_cwjap(const char *str, esp_msg_t *msg);
uint8_t espi_parse_cwlif(const char *str, esp_msg_t *msg);
uint8_t espi_parse_cipdomain(const char *src, esp_msg_t *msg);
uint8_t espi_parse_cipsntptime(const char *str, esp_msg_t *msg);
uint8_t espi_parse_ping_time(const char *str, esp_msg_t *msg);
uint8_t espi_parse_hostname(const char *str, esp_msg_t *msg);
uint8_t espi_parse_link_conn(const char *str);

uint8_t espi_parse_at_sdk_version(const char *str, esp_sw_version_t *version_out);

uint8_t espi_parse_ap_conn_disconn_sta(const char *str, uint8_t is_conn);
uint8_t espi_parse_ap_ip_sta(const char *str);
uint8_t espi_parse_cwsap(const char *str, esp_msg_t *msg);

uint8_t espi_parse_cwdhcp(const char *str);

#ifdef __cplusplus
}
#endif /* __cplusplus */
