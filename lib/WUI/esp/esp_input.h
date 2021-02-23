#include <string.h>
#include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         ESP
 * \defgroup        ESP_INPUT Input processing
 * \brief           Input function for received data
 * \{
 */

esp_res_t    esp_input(const void* data, size_t len);
esp_res_t    esp_input_process(const void* data, size_t len);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

