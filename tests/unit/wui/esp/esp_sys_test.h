#include <stdint.h>
#include "esp_opt.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        ESP_SYS System functions
 * \brief           System based function for OS management, timings, etc
 * \{
 */

/**
 * \brief           Thread function prototype
 */
typedef void (*esp_sys_thread_fn)(void*);

/* Include system port file from portable folder */
#include "esp_sys_port_test.h"

/**
 * \anchor          ESP_SYS_CORE
 * \name            Main
 */

uint8_t     esp_sys_init(void);
uint32_t    esp_sys_now(void);

uint8_t     esp_sys_protect(void);
uint8_t     esp_sys_unprotect(void);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_MUTEX
 * \name            Mutex
 */

uint8_t     esp_sys_mutex_create(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_delete(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_lock(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_unlock(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_isvalid(esp_sys_mutex_t* p);
uint8_t     esp_sys_mutex_invalid(esp_sys_mutex_t* p);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_SEM
 * \name            Semaphores
 */

uint8_t     esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt);
uint8_t     esp_sys_sem_delete(esp_sys_sem_t* p);
uint32_t    esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout);
uint8_t     esp_sys_sem_release(esp_sys_sem_t* p);
uint8_t     esp_sys_sem_isvalid(esp_sys_sem_t* p);
uint8_t     esp_sys_sem_invalid(esp_sys_sem_t* p);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_MBOX
 * \name            Message queues
 */

uint8_t     esp_sys_mbox_create(esp_sys_mbox_t* b, size_t size);
uint8_t     esp_sys_mbox_delete(esp_sys_mbox_t* b);
uint32_t    esp_sys_mbox_put(esp_sys_mbox_t* b, void* m);
uint32_t    esp_sys_mbox_get(esp_sys_mbox_t* b, void** m, uint32_t timeout);
uint8_t     esp_sys_mbox_putnow(esp_sys_mbox_t* b, void* m);
uint8_t     esp_sys_mbox_getnow(esp_sys_mbox_t* b, void** m);
uint8_t     esp_sys_mbox_isvalid(esp_sys_mbox_t* b);
uint8_t     esp_sys_mbox_invalid(esp_sys_mbox_t* b);

/**
 * \}
 */

/**
 * \anchor          ESP_SYS_THREAD
 * \name            Threads
 */

uint8_t     esp_sys_thread_create(esp_sys_thread_t* t, const char* name, esp_sys_thread_fn thread_func, void* const arg, size_t stack_size, esp_sys_thread_prio_t prio);
uint8_t     esp_sys_thread_terminate(esp_sys_thread_t* t);
uint8_t     esp_sys_thread_yield(void);

/**
 * \}
 */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

