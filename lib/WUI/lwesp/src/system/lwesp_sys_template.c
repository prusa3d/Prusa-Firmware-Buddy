/**
 * \file            lwesp_sys_template.c
 * \brief           System dependant functions
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.0.0
 */
#include "system/lwesp_sys.h"
#include "cmsis_os.h"

static osMutexId sys_mutex;

/**
 * \brief           Init system dependant parameters
 *
 * After this function is called,
 * all other system functions must be fully ready.
 *
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_init(void) {
    lwesp_sys_mutex_create(&sys_mutex);
    return 1;
}

/**
 * \brief           Get current time in units of milliseconds
 * \return          Current time in units of milliseconds
 */
uint32_t
lwesp_sys_now(void) {
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
uint8_t
lwesp_sys_protect(void) {
    lwesp_sys_mutex_lock(&sys_mutex);
    return 1;
}

/**
 * \brief           Unprotect middleware core
 *
 * This function must follow number of calls of \ref lwesp_sys_protect
 * and unlock access only when counter reached back zero.
 *
 * \note            Most operating systems support recursive mutexes.
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_unprotect(void) {
    lwesp_sys_mutex_unlock(&sys_mutex);
    return 1;
}

/**
 * \brief           Create new recursive mutex
 * \note            Recursive mutex has to be created as it may be locked multiple times before unlocked
 * \param[out]      p: Pointer to mutex structure to allocate
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mutex_create(lwesp_sys_mutex_t* p) {
    osMutexDef(MUT);
    *p = osRecursiveMutexCreate(osMutex(MUT));
    return *p != NULL;
}

/**
 * \brief           Delete recursive mutex from system
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mutex_delete(lwesp_sys_mutex_t* p) {
    return osMutexDelete(*p) == osOK;
}

/**
 * \brief           Lock recursive mutex, wait forever to lock
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mutex_lock(lwesp_sys_mutex_t* p) {
    return osRecursiveMutexWait(*p, osWaitForever) == osOK;
}

/**
 * \brief           Unlock recursive mutex
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mutex_unlock(lwesp_sys_mutex_t* p) {
    return osRecursiveMutexRelease(*p) == osOK;
}

/**
 * \brief           Check if mutex structure is valid system
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mutex_isvalid(lwesp_sys_mutex_t* p) {
    return p != NULL && *p != NULL;
}

/**
 * \brief           Set recursive mutex structure as invalid
 * \param[in]       p: Pointer to mutex structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mutex_invalid(lwesp_sys_mutex_t* p) {
    *p = LWESP_SYS_MUTEX_NULL;
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
uint8_t
lwesp_sys_sem_create(lwesp_sys_sem_t* p, uint8_t cnt) {
    osSemaphoreDef(SEM);
    *p = osSemaphoreCreate(osSemaphore(SEM), 1);

    if (*p != NULL && !cnt) {
        osSemaphoreWait(*p, 0);
    }
    return *p != NULL;
}

/**
 * \brief           Delete binary semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_sem_delete(lwesp_sys_sem_t* p) {
    return osSemaphoreDelete(*p) == osOK;
}

/**
 * \brief           Wait for semaphore to be available
 * \param[in]       p: Pointer to semaphore structure
 * \param[in]       timeout: Timeout to wait in milliseconds. When `0` is applied, wait forever
 * \return          Number of milliseconds waited for semaphore to become available or
 *                      \ref LWESP_SYS_TIMEOUT if not available within given time
 */
uint32_t
lwesp_sys_sem_wait(lwesp_sys_sem_t* p, uint32_t timeout) {
    uint32_t tick = osKernelSysTick();
    return (osSemaphoreWait(*p, !timeout ? osWaitForever : timeout) == osOK) ? (osKernelSysTick() - tick) : LWESP_SYS_TIMEOUT;
}

/**
 * \brief           Release semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_sem_release(lwesp_sys_sem_t* p) {
    return osSemaphoreRelease(*p) == osOK;
}

/**
 * \brief           Check if semaphore is valid
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_sem_isvalid(lwesp_sys_sem_t* p) {
    return p != NULL && *p != NULL;
}

/**
 * \brief           Invalid semaphore
 * \param[in]       p: Pointer to semaphore structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_sem_invalid(lwesp_sys_sem_t* p) {
    *p = LWESP_SYS_SEM_NULL;
    return 1;
}

/**
 * \brief           Create a new message queue with entry type of `void *`
 * \param[out]      b: Pointer to message queue structure
 * \param[in]       size: Number of entries for message queue to hold
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mbox_create(lwesp_sys_mbox_t* b, size_t size) {
    osMessageQDef(MBOX, size, void*);
    *b = osMessageCreate(osMessageQ(MBOX), NULL);
    return *b != NULL;
}

/**
 * \brief           Delete message queue
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mbox_delete(lwesp_sys_mbox_t* b) {
    if (osMessageWaiting(*b)) {
        return 0;
    }
    return osMessageDelete(*b) == osOK;
}

/**
 * \brief           Put a new entry to message queue and wait until memory available
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to entry to insert to message queue
 * \return          Time in units of milliseconds needed to put a message to queue
 */
uint32_t
lwesp_sys_mbox_put(lwesp_sys_mbox_t* b, void* m) {
    uint32_t tick = osKernelSysTick();
    return osMessagePut(*b, (uint32_t)m, osWaitForever) == osOK ? (osKernelSysTick() - tick) : LWESP_SYS_TIMEOUT;
}

/**
 * \brief           Get a new entry from message queue with timeout
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \param[in]       timeout: Maximal timeout to wait for new message. When `0` is applied, wait for unlimited time
 * \return          Time in units of milliseconds needed to put a message to queue
 *                      or \ref LWESP_SYS_TIMEOUT if it was not successful
 */
uint32_t
lwesp_sys_mbox_get(lwesp_sys_mbox_t* b, void** m, uint32_t timeout) {
    osEvent evt;
    uint32_t time = osKernelSysTick();

    evt = osMessageGet(*b, !timeout ? osWaitForever : timeout);
    if (evt.status == osEventMessage) {
        *m = evt.value.p;
        return osKernelSysTick() - time;
    }
    return LWESP_SYS_TIMEOUT;
}

/**
 * \brief           Put a new entry to message queue without timeout (now or fail)
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to message to save to queue
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mbox_putnow(lwesp_sys_mbox_t* b, void* m) {
    return osMessagePut(*b, (uint32_t)m, 0) == osOK;
}

/**
 * \brief           Get an entry from message queue immediately
 * \param[in]       b: Pointer to message queue structure
 * \param[in]       m: Pointer to pointer to result to save value from message queue to
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mbox_getnow(lwesp_sys_mbox_t* b, void** m) {
    osEvent evt;

    evt = osMessageGet(*b, 0);
    if (evt.status == osEventMessage) {
        *m = evt.value.p;
        return 1;
    }
    return 0;
}

/**
 * \brief           Check if message queue is valid
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mbox_isvalid(lwesp_sys_mbox_t* b) {
    return b != NULL && *b != NULL;
}

/**
 * \brief           Invalid message queue
 * \param[in]       b: Pointer to message queue structure
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_mbox_invalid(lwesp_sys_mbox_t* b) {
    *b = LWESP_SYS_MBOX_NULL;
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
uint8_t
lwesp_sys_thread_create(lwesp_sys_thread_t* t, const char* name, lwesp_sys_thread_fn thread_func,
                      void* const arg, size_t stack_size, lwesp_sys_thread_prio_t prio) {
    const osThreadDef_t thread_def = {
        (char*)name,
        (os_pthread)thread_func,
        (osPriority)prio,
        0,
        stack_size > 0 ? stack_size : LWESP_SYS_THREAD_SS
    };
    lwesp_sys_thread_t id;

    id = osThreadCreate(&thread_def, arg);
    if (t != NULL) {
        *t = id;
    }
    return id != NULL;
}

/**
 * \brief           Terminate thread (shut it down and remove)
 * \param[in]       t: Pointer to thread handle to terminate.
 *                      If set to `NULL`, terminate current thread (thread from where function is called)
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_thread_terminate(lwesp_sys_thread_t* t) {
    osThreadTerminate(t != NULL ? *t : NULL);
    return 1;
}

/**
 * \brief           Yield current thread
 * \return          `1` on success, `0` otherwise
 */
uint8_t
lwesp_sys_thread_yield(void) {
    osThreadYield();
    return 1;
}
