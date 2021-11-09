/**
 * @file
 * netbuf API (for netconn API)
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Modified to provide sockets on top of LwESP netconn.
 *
 */
#ifndef LWESP_HDR_NETBUF_H
#define LWESP_HDR_NETBUF_H

#include "lwip/opt.h"

#if LWIP_NETCONN || LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */
/* Note: Netconn API is always available when sockets are enabled -
 * sockets are implemented on top of them */

#include "lwip/err.h"
#include "esp/esp_pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/ip6_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** This netbuf has dest-addr/port set */
#define NETBUF_FLAG_DESTADDR    0x01
/** This netbuf includes a checksum */
#define NETBUF_FLAG_CHKSUM      0x02

/** "Network buffer" - contains data and addressing info */
struct lwesp_netbuf {
  struct esp_pbuf *p, *ptr;
  ip_addr_t addr;
  u16_t port;
#if LWIP_NETBUF_RECVINFO || LWIP_CHECKSUM_ON_COPY
  u8_t flags;
  u16_t toport_chksum;
#if LWIP_NETBUF_RECVINFO
  ip_addr_t toaddr;
#endif /* LWIP_NETBUF_RECVINFO */
#endif /* LWIP_NETBUF_RECVINFO || LWIP_CHECKSUM_ON_COPY */
};

/* Network buffer functions: */
struct lwesp_netbuf *   lwesp_netbuf_new      (void);
void              lwesp_netbuf_delete   (struct lwesp_netbuf *buf);
const void *            lwesp_netbuf_alloc    (struct lwesp_netbuf *buf, u16_t size);
void              lwesp_netbuf_free     (struct lwesp_netbuf *buf);
err_t             lwesp_netbuf_ref      (struct lwesp_netbuf *buf,
                                   const void *dataptr, u16_t size);
void              lwesp_netbuf_chain    (struct lwesp_netbuf *head, struct lwesp_netbuf *tail);

err_t             lwesp_netbuf_data     (struct lwesp_netbuf *buf,
                                   const void **dataptr, u16_t *len);
s8_t              lwesp_netbuf_next     (struct lwesp_netbuf *buf);
void              lwesp_netbuf_first    (struct lwesp_netbuf *buf);


#define lwesp_netbuf_copy_partial(buf, dataptr, len, offset) \
  esp_pbuf_copy_partial((buf)->p, (dataptr), (len), (offset))
#define lwesp_netbuf_copy(buf,dataptr,len) lwesp_netbuf_copy_partial(buf, dataptr, len, 0)
#define lwesp_netbuf_take(buf, dataptr, len) lwesp_pbuf_take((buf)->p, dataptr, len)
#define lwesp_netbuf_len(buf)              ((buf)->p->tot_len)
#define lwesp_netbuf_fromaddr(buf)         (&((buf)->addr))
#define lwesp_netbuf_set_fromaddr(buf, fromaddr) ip_addr_set(&((buf)->addr), fromaddr)
#define lwesp_netbuf_fromport(buf)         ((buf)->port)
#if LWIP_NETBUF_RECVINFO
#define lwesp_netbuf_destaddr(buf)         (&((buf)->toaddr))
#define lwesp_netbuf_set_destaddr(buf, destaddr) ip_addr_set(&((buf)->toaddr), destaddr)
#if LWIP_CHECKSUM_ON_COPY
#define lwesp_netbuf_destport(buf)         (((buf)->flags & NETBUF_FLAG_DESTADDR) ? (buf)->toport_chksum : 0)
#else /* LWIP_CHECKSUM_ON_COPY */
#define lwesp_netbuf_destport(buf)         ((buf)->toport_chksum)
#endif /* LWIP_CHECKSUM_ON_COPY */
#endif /* LWIP_NETBUF_RECVINFO */
#if LWIP_CHECKSUM_ON_COPY
#define lwesp_netbuf_set_chksum(buf, chksum) do { (buf)->flags = NETBUF_FLAG_CHKSUM; \
                                            (buf)->toport_chksum = chksum; } while(0)
#endif /* LWIP_CHECKSUM_ON_COPY */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_NETCONN || LWIP_SOCKET */

#endif /* LWESP_HDR_NETBUF_H */
