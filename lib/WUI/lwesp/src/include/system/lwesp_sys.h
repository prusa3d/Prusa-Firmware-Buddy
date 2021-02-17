/**
 * \file            lwesp_sys.h
 * \brief           Main system include file which decides later include file
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
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.0.0
 */
#ifndef LWESP_HDR_MAIN_SYS_H
#define LWESP_HDR_MAIN_SYS_H

#include <stdint.h>
#include "lwesp/lwesp_opt.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        LWESP_SYS System functions
 * \brief           System based function for OS management, timings, etc
 * \{
 */

/**
 * \brief           Thread function prototype
 */
typedef void (*lwesp_sys_thread_fn)(void*);

/* Include system port file from portable folder */
#include "lwesp_sys_port.h"

/**
 * \anchor          LWESP_SYS_CORE
 * \name            Main
 */

uint8_t     lwesp_sys_init(void);
uint32_t    lwesp_sys_now(void);

uint8_t     lwesp_sys_protect(void);
uint8_t     lwesp_sys_unprotect(void);

/**
 * \}
 */

/**
 * \anchor          LWESP_SYS_MUTEX
 * \name            Mutex
 */

uint8_t     lwesp_sys_mutex_create(lwesp_sys_mutex_t* p);
uint8_t     lwesp_sys_mutex_delete(lwesp_sys_mutex_t* p);
uint8_t     lwesp_sys_mutex_lock(lwesp_sys_mutex_t* p);
uint8_t     lwesp_sys_mutex_unlock(lwesp_sys_mutex_t* p);
uint8_t     lwesp_sys_mutex_isvalid(lwesp_sys_mutex_t* p);
uint8_t     lwesp_sys_mutex_invalid(lwesp_sys_mutex_t* p);

/**
 * \}
 */

/**
 * \anchor          LWESP_SYS_SEM
 * \name            Semaphores
 */

uint8_t     lwesp_sys_sem_create(lwesp_sys_sem_t* p, uint8_t cnt);
uint8_t     lwesp_sys_sem_delete(lwesp_sys_sem_t* p);
uint32_t    lwesp_sys_sem_wait(lwesp_sys_sem_t* p, uint32_t timeout);
uint8_t     lwesp_sys_sem_release(lwesp_sys_sem_t* p);
uint8_t     lwesp_sys_sem_isvalid(lwesp_sys_sem_t* p);
uint8_t     lwesp_sys_sem_invalid(lwesp_sys_sem_t* p);

/**
 * \}
 */

/**
 * \anchor          LWESP_SYS_MBOX
 * \name            Message queues
 */

uint8_t     lwesp_sys_mbox_create(lwesp_sys_mbox_t* b, size_t size);
uint8_t     lwesp_sys_mbox_delete(lwesp_sys_mbox_t* b);
uint32_t    lwesp_sys_mbox_put(lwesp_sys_mbox_t* b, void* m);
uint32_t    lwesp_sys_mbox_get(lwesp_sys_mbox_t* b, void** m, uint32_t timeout);
uint8_t     lwesp_sys_mbox_putnow(lwesp_sys_mbox_t* b, void* m);
uint8_t     lwesp_sys_mbox_getnow(lwesp_sys_mbox_t* b, void** m);
uint8_t     lwesp_sys_mbox_isvalid(lwesp_sys_mbox_t* b);
uint8_t     lwesp_sys_mbox_invalid(lwesp_sys_mbox_t* b);

/**
 * \}
 */

/**
 * \anchor          LWESP_SYS_THREAD
 * \name            Threads
 */

uint8_t     lwesp_sys_thread_create(lwesp_sys_thread_t* t, const char* name, lwesp_sys_thread_fn thread_func, void* const arg, size_t stack_size, lwesp_sys_thread_prio_t prio);
uint8_t     lwesp_sys_thread_terminate(lwesp_sys_thread_t* t);
uint8_t     lwesp_sys_thread_yield(void);

/**
 * \}
 */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_HDR_MAIN_SYS_H */
