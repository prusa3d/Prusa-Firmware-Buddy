#include <stdint.h>
#include <stdlib.h>
#include "esp/esp_opt.h"
// #include "FreeRTOS.h"
// #include "task.h"
// #include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if ESP_CFG_OS && !__DOXYGEN__

typedef SemaphoreHandle_t           esp_sys_mutex_t;
typedef SemaphoreHandle_t           esp_sys_sem_t;
typedef QueueHandle_t               esp_sys_mbox_t;
typedef TaskHandle_t                esp_sys_thread_t;
typedef UBaseType_t                 esp_sys_thread_prio_t;

#define ESP_SYS_MUTEX_NULL          ((SemaphoreHandle_t)0)
#define ESP_SYS_SEM_NULL            ((SemaphoreHandle_t)0)
#define ESP_SYS_MBOX_NULL           ((QueueHandle_t)0)
#define ESP_SYS_TIMEOUT             ((TickType_t)portMAX_DELAY)
#define ESP_SYS_THREAD_PRIO         (configMAX_PRIORITIES - 1)
#define ESP_SYS_THREAD_SS           (1024)

#endif /* ESP_CFG_OS && !__DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

