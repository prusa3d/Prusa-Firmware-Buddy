#include "esp_sys_test.h"
#include <chrono>
// #include <queue>
// #include <mutex>
// #include <thread>

std::mutex sys_mutex;

uint32_t osKernelSysTick(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

/**
 * \brief           Init system dependant parameters
 *
 * After this function is called,
 * all other system functions must be fully ready.
 *
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_init(void) {
    esp_sys_mutex_create(&sys_mutex);
    return 1;
}

/**
 * \brief           Get current time in units of milliseconds
 * \return          Current time in units of milliseconds
 */
uint32_t esp_sys_now(void) {
    return osKernelSysTick();
}

/**
 * \brief           Protect middleware core
 *
 * Stack protection must support recursive mode.
 * This function may be called multiple times,
 * even if access has been granted before.
 *
 * \note            Most operating systems support recursive mutexes.
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_protect(void) {
    esp_sys_mutex_lock(&sys_mutex);
    return 1;
}

/**
 * \brief           Unprotect middleware core
 *
 * This function must follow number of calls of \ref esp_sys_protect
 * and unlock access only when counter reached back zero.
 *
 * \note            Most operating systems support recursive mutexes.
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_unprotect(void) {
    esp_sys_mutex_unlock(&sys_mutex);
    return 1;
}

/**
 * \brief           Create new recursive mutex
 * \note            Recursive mutex has to be created as it may be locked multiple times before unlocked
 * \param[out]      p: Pointer to mutex structure to allocate
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mutex_create(esp_sys_mutex_t *p) {
    // *p = &sys_mutex;
    // return *p != NULL;
    // return *sys_mutex != NULL
    return 1;
}

/**
 * \brief           Delete recursive mutex from system
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mutex_delete(esp_sys_mutex_t *p) {
    // return osMutexDelete(*p) == osOK;
    return 1;
}

/**
 * \brief           Lock recursive mutex, wait forever to lock
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mutex_lock(esp_sys_mutex_t *p) {
    // std::lock_guard<std::mutex> lock(p);
    p->lock();
    return 1;
}

/**
 * \brief           Unlock recursive mutex
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mutex_unlock(esp_sys_mutex_t *p) {
    p->unlock();
    return 1;
    // return osRecursiveMutexRelease(*p) == osOK;
}

/**
 * \brief           Check if mutex structure is valid system
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mutex_isvalid(esp_sys_mutex_t *p) {
    // return p != NULL && *p != NULL;
    return 1;
}

/**
 * \brief           Set recursive mutex structure as invalid
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mutex_invalid(esp_sys_mutex_t *p) {
    // *p = ESP_SYS_MUTEX_NULL;
    return 1;
}

/**
 * \brief           Create a new binary semaphore and set initial state
 * \note            Semaphore may only have `1` token available
 * \param[out]      p: Pointer to semaphore structure to fill with result
 * \param[in]       cnt: Count indicating default semaphore state:
 *                     `0`: Take semaphore token immediately
 *                     `1`: Keep token available
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_sem_create(esp_sys_sem_t *p, uint8_t cnt) {
    // osSemaphoreDef(SEM);
    // *p = osSemaphoreCreate(osSemaphore(SEM), 1);
    //
    // if (*p != NULL && !cnt) {
    // osSemaphoreWait(*p, 0);
    // }
    // return *p != NULL;
    return 1;
}

/**
 * \brief           Delete binary semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_sem_delete(esp_sys_sem_t *p) {
    // return osSemaphoreDelete(*p) == osOK;
    return 1;
}

/**
 * \brief           Wait for semaphore to be available
 * \param[in]       p: Pointer to semaphore structure
 * \param[in]       timeout: Timeout to wait in milliseconds. When `0` is applied, wait forever
 * \return          Number of milliseconds waited for semaphore to become available or
 *                      \ref ESP_SYS_TIMEOUT if not available within given time
 */
uint32_t esp_sys_sem_wait(esp_sys_sem_t *p, uint32_t timeout) {
    // uint32_t tick = osKernelSysTick();
    // return (osSemaphoreWait(*p, !timeout ? osWaitForever : timeout) == osOK) ? (osKernelSysTick() - tick) : ESP_SYS_TIMEOUT;
    return 1;
}

/**
 * \brief           Release semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_sem_release(esp_sys_sem_t *p) {
    // return osSemaphoreRelease(*p) == osOK;
    return 1;
}

/**
 * \brief           Check if semaphore is valid
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_sem_isvalid(esp_sys_sem_t *p) {
    // return p != NULL && *p != NULL;
    return 1;
}

/**
 * \brief           Invalid semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_sem_invalid(esp_sys_sem_t *p) {
    // *p = ESP_SYS_SEM_NULL;
    return 1;
}

/**
 * \brief           Create a new message queue with entry type of `void *`
 * \param[out]      b: Pointer to message queue structure
 * \param[in]       size: Number of entries for message queue to hold
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mbox_create(esp_sys_mbox_t *b, size_t size) {
    // osMessageQDef(MBOX, size, void *);
    // *b = osMessageCreate(osMessageQ(MBOX), NULL);
    // return *b != NULL;
    return 1;
}

/**
 * \brief           Delete message queue
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mbox_delete(esp_sys_mbox_t *b) {
    // if (osMessageWaiting(*b)) {
    // return 0;
    // }
    // return osMessageDelete(*b) == osOK;
    return 1;
}

/**
 * \brief           Put a new entry to message queue and wait until memory available
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to entry to insert to message queue
 * \return          Time in units of milliseconds needed to put a message to queue
 */
uint32_t esp_sys_mbox_put(esp_sys_mbox_t *b, void *m) {
    // uint32_t tick = osKernelSysTick();
    // return osMessagePut(*b, (uint32_t)m, osWaitForever) == osOK ? (osKernelSysTick() - tick) : ESP_SYS_TIMEOUT;
    return 1;
}

/**
 * \brief           Get a new entry from message queue with timeout
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \param[in]       timeout: Maximal timeout to wait for new message. When `0` is applied, wait for unlimited time
 * \return          Time in units of milliseconds needed to put a message to queue
 *                      or \ref ESP_SYS_TIMEOUT if it was not successful
 */
uint32_t esp_sys_mbox_get(esp_sys_mbox_t *b, void **m, uint32_t timeout) {
    // osEvent evt;
    // uint32_t time = osKernelSysTick();
    //
    // evt = osMessageGet(*b, !timeout ? osWaitForever : timeout);
    // if (evt.status == osEventMessage) {
    // *m = evt.value.p;
    // return osKernelSysTick() - time;
    // }
    return ESP_SYS_TIMEOUT;
}

/**
 * \brief           Put a new entry to message queue without timeout (now or fail)
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to message to save to queue
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mbox_putnow(esp_sys_mbox_t *b, void *m) {
    // return osMessagePut(*b, (uint32_t)m, 0) == osOK;
    return 1;
}

/**
 * \brief           Get an entry from message queue immediately
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mbox_getnow(esp_sys_mbox_t *b, void **m) {
    // osEvent evt;
    //
    // evt = osMessageGet(*b, 0);
    // if (evt.status == osEventMessage) {
    // *m = evt.value.p;
    // return 1;
    // }
    // return 0;
    return 1;
}

/**
 * \brief           Check if message queue is valid
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mbox_isvalid(esp_sys_mbox_t *b) {
    // return b != NULL && *b != NULL;
    return 0;
}

/**
 * \brief           Invalid message queue
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_mbox_invalid(esp_sys_mbox_t *b) {
    // *b = ESP_SYS_MBOX_NULL;
    return 1;
}

/**
 * \brief           Create a new thread
 * \param[out]      t: Pointer to thread identifier if create was successful.
 *                     It may be set to `NULL`
 * \param[in]       name: Name of a new thread
 * \param[in]       thread_func: Thread function to use as thread body
 * \param[in]       arg: Thread function argument
 * \param[in]       stack_size: Size of thread stack in uints of bytes. If set to 0, reserve default stack size
 * \param[in]       prio: Thread priority
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_thread_create(esp_sys_thread_t *t, const char *name, esp_sys_thread_fn thread_func,
    void *const arg, size_t stack_size, esp_sys_thread_prio_t prio) {
    // const osThreadDef_t thread_def = {
    // (char *)name,
    // (os_pthread)thread_func,
    // (osPriority)prio,
    // 0,
    // stack_size > 0 ? stack_size : ESP_SYS_THREAD_SS
    // };
    // esp_sys_thread_t id;
    //
    // id = osThreadCreate(&thread_def, arg);
    // if (t != NULL) {
    // *t = id;
    // }

    // t = std::thread(thread_func, 1);
    return 1;

    // std::thread id = std::thread(thread_func, 1);
    // if (t != NULL) {
        // *t = &id;
    // }
    // return id != NULL;
}

/**
 * \brief           Terminate thread (shut it down and remove)
 * \param[in]       t: Pointer to thread handle to terminate.
 *                      If set to `NULL`, terminate current thread (thread from where function is called)
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_thread_terminate(esp_sys_thread_t *t) {
    t->join();
    // osThreadTerminate(t != NULL ? *t : NULL);
    delete t;
    return 1;
}

/**
 * \brief           Yield current thread
 * \return          `1` on success, `0` otherwise
 */
uint8_t esp_sys_thread_yield(void) {
    // osThreadYield();
    return 1;
}
