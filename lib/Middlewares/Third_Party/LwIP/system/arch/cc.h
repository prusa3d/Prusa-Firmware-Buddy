/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __CC_H__
#define __CC_H__

#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

typedef int sys_prot_t;

//#define LWIP_PROVIDE_ERRNO
#define LWIP_ERRNO_INCLUDE <sys/errno.h>

#if defined (__GNUC__) & !defined (__CC_ARM)

#define LWIP_TIMEVAL_PRIVATE 0
#include <sys/time.h>

#endif

/* define compiler specific symbols */
#if defined (__ICCARM__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT 
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES

#elif defined (__GNUC__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__CC_ARM)

#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#elif defined (__TASKING__)

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif

#define DEPAREN(X) ESC(ISH X)
#define ISH(...) ISH __VA_ARGS__
#define ESC(...) ESC_(__VA_ARGS__)
#define ESC_(...) VAN ## __VA_ARGS__
#define VANISH

void lwip_platform_log_info(const char *fmt, ...);

#define LWIP_PLATFORM_DIAG(x) lwip_platform_log_info(DEPAREN(x))

#ifdef _DEBUG
#define LWIP_PLATFORM_ASSERT(x)                                             \
    do {                                                                    \
        extern void lwip_platform_assert(const char*, const char*, int);    \
        lwip_platform_assert(x, __FILE__, __LINE__);                        \
    } while (0)
#else
#define LWIP_PLATFORM_ASSERT(x)                                             \
    do {                                                                    \
        extern void lwip_platform_assert(const char*, const char*, int);    \
        lwip_platform_assert(x, "<unknown>", 0);                            \
    } while (0)
#endif


#define LWIP_ERROR(message, expression, handler) \
    do {                                         \
        if (!(expression)) {                     \
            extern void lwip_platform_log_error(const char*); \
            lwip_platform_log_error(message); \
            handler;                             \
        }                                        \
    } while (0)

/* Define random number generator function */
#define LWIP_RAND() ((u32_t)rand())

extern uint8_t __attribute__((section(".ccmram"))) memp_memory_NETBUF_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_TCP_SEG_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_SYS_TIMEOUT_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_NETCONN_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_UDP_PCB_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_FRAG_PBUF_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_TCPIP_MSG_API_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_TCP_PCB_base[];
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_PBUF_base[];
#if 0
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_PBUF_POOL_base[];
#endif
extern uint8_t __attribute__((section(".ccmram"))) memp_memory_TCPIP_MSG_INPKT_base[];
extern uint8_t memp_memory_TCP_PCB_LISTEN_base[];
extern uint8_t memp_memory_REASSDATA_base[];

#endif /* __CC_H__ */
