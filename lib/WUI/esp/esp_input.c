#include "esp/esp_private.h"
#include "esp/esp.h"
#include "esp/esp_input.h"
#include "esp/esp_buff.h"

static uint32_t esp_recv_total_len;
static uint32_t esp_recv_calls;

#if !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__

/**
 * \brief           Write data to input buffer
 * \note            \ref ESP_CFG_INPUT_USE_PROCESS must be disabled to use this function
 * \param[in]       data: Pointer to data to write
 * \param[in]       len: Number of data elements in units of bytes
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_input(const void* data, size_t len) {
    if (!esp.status.f.initialized || esp.buff.buff == NULL) {
        return espERR;
    }
    esp_buff_write(&esp.buff, data, len);     /* Write data to buffer */
    esp_sys_mbox_putnow(&esp.mbox_process, NULL); /* Write empty box, don't care if write fails */
    esp_recv_total_len += len;                /* Update total number of received bytes */
    ++esp_recv_calls;                         /* Update number of calls */
    return espOK;
}

#endif /* !ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__ */

#if ESP_CFG_INPUT_USE_PROCESS || __DOXYGEN__

/**
 * \brief           Process input data directly without writing it to input buffer
 * \note            This function may only be used when in OS mode,
 *                  where single thread is dedicated for input read of AT receive
 *
 * \note            \ref ESP_CFG_INPUT_USE_PROCESS must be enabled to use this function
 *
 * \param[in]       data: Pointer to received data to be processed
 * \param[in]       len: Length of data to process in units of bytes
 * \return          \ref espOK on success, member of \ref esp_res_t enumeration otherwise
 */
esp_res_t
esp_input_process(const void* data, size_t len) {
    esp_res_t res = espOK;

    if (!esp.status.f.initialized) {
        return espERR;
    }

    esp_recv_total_len += len;                /* Update total number of received bytes */
    ++esp_recv_calls;                         /* Update number of calls */

    if (len > 0) {
        esp_core_lock();
        res = espi_process(data, len);        /* Process input data */
        esp_core_unlock();
    }
    return res;
}

