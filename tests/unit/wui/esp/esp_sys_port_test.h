#include <queue>
#include <mutex>
#include <thread>
#include "esp/esp_opt.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if ESP_CFG_OS && !__DOXYGEN__

typedef std::mutex esp_sys_mutex_t;
typedef std::mutex esp_sys_sem_t;
typedef std::queue esp_sys_mbox_t;
typedef std::thread esp_sys_thread_t;
typedef int esp_sys_thread_prio_t;

    #define ESP_SYS_MBOX_NULL   ((std::queue)0)
    #define ESP_SYS_SEM_NULL    ((std::mutex)0)
    #define ESP_SYS_MUTEX_NULL  ((std::mutex)0)
    #define ESP_SYS_TIMEOUT     (1000)
    #define ESP_SYS_THREAD_PRIO (0)
    #define ESP_SYS_THREAD_SS   (1024)

#endif /* ESP_CFG_OS && !__DOXYGEN__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */
