// #include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         ESP
 * \defgroup        ESP_PBUF Packet buffer
 * \brief           Packet buffer manager
 * \{
 */

esp_pbuf_p esp_pbuf_new(size_t len);
size_t esp_pbuf_free(esp_pbuf_p pbuf);
void *esp_pbuf_data(const esp_pbuf_p pbuf);
size_t esp_pbuf_length(const esp_pbuf_p pbuf, uint8_t tot);
uint8_t esp_pbuf_set_length(esp_pbuf_p pbuf, size_t new_len);
esp_res_t esp_pbuf_take(esp_pbuf_p pbuf, const void *data, size_t len, size_t offset);
size_t esp_pbuf_copy(esp_pbuf_p pbuf, void *data, size_t len, size_t offset);

esp_res_t esp_pbuf_cat(esp_pbuf_p head, const esp_pbuf_p tail);
esp_res_t esp_pbuf_chain(esp_pbuf_p head, esp_pbuf_p tail);
esp_pbuf_p esp_pbuf_unchain(esp_pbuf_p head);
esp_res_t esp_pbuf_ref(esp_pbuf_p pbuf);

uint8_t esp_pbuf_get_at(const esp_pbuf_p pbuf, size_t pos, uint8_t *el);
size_t esp_pbuf_memcmp(const esp_pbuf_p pbuf, const void *data, size_t len, size_t offset);
size_t esp_pbuf_strcmp(const esp_pbuf_p pbuf, const char *str, size_t offset);
size_t esp_pbuf_memfind(const esp_pbuf_p pbuf, const void *data, size_t len, size_t off);
size_t esp_pbuf_strfind(const esp_pbuf_p pbuf, const char *str, size_t off);

uint8_t esp_pbuf_advance(esp_pbuf_p pbuf, int len);
esp_pbuf_p esp_pbuf_skip(esp_pbuf_p pbuf, size_t offset, size_t *new_offset);

void *esp_pbuf_get_linear_addr(const esp_pbuf_p pbuf, size_t offset, size_t *new_len);

void esp_pbuf_set_ip(esp_pbuf_p pbuf, const esp_ip_t *ip, esp_port_t port);

void esp_pbuf_dump(esp_pbuf_p p, uint8_t seq);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
