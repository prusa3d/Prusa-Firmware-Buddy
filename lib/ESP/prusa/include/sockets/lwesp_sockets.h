/**
 * @file
 * Socket API (to be used from non-TCPIP threads)
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


#ifndef LWESP_HDR_SOCKETS_H
#define LWESP_HDR_SOCKETS_H

#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/errno.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int lwesp_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwesp_bind(int s, const struct sockaddr *name, socklen_t namelen);
int lwesp_shutdown(int s, int how);
int lwesp_getpeername (int s, struct sockaddr *name, socklen_t *namelen);
int lwesp_getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int lwesp_getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen);
int lwesp_setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen);
 int lwesp_close(int s);
int lwesp_connect(int s, const struct sockaddr *name, socklen_t namelen);
int lwesp_listen(int s, int backlog);
ssize_t lwesp_recv(int s, void *mem, size_t len, int flags);
ssize_t lwesp_read(int s, void *mem, size_t len);
ssize_t lwesp_readv(int s, const struct iovec *iov, int iovcnt);
ssize_t lwesp_recvfrom(int s, void *mem, size_t len, int flags,
      struct sockaddr *from, socklen_t *fromlen);
ssize_t lwesp_recvmsg(int s, struct msghdr *message, int flags);
ssize_t lwesp_send(int s, const void *dataptr, size_t size, int flags);
ssize_t lwesp_sendmsg(int s, const struct msghdr *message, int flags);
ssize_t lwesp_sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);
int lwesp_socket(int domain, int type, int protocol);
ssize_t lwesp_write(int s, const void *dataptr, size_t size);
ssize_t lwesp_writev(int s, const struct iovec *iov, int iovcnt);
#if LWIP_SOCKET_SELECT
int lwesp_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
                struct timeval *timeout);
#endif
#if LWIP_SOCKET_POLL
int lwesp_poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif
int lwesp_ioctl(int s, long cmd, void *argp);
int lwesp_fcntl(int s, int cmd, int val);
const char *lwesp_inet_ntop(int af, const void *src, char *dst, socklen_t size);
int lwesp_inet_pton(int af, const char *src, void *dst);

#if LWIP_COMPAT_SOCKETS
#if LWIP_COMPAT_SOCKETS != 2
/** @ingroup socket */
#define accept(s,addr,addrlen)                    lwip_accept(s,addr,addrlen)
/** @ingroup socket */
#define bind(s,name,namelen)                      lwip_bind(s,name,namelen)
/** @ingroup socket */
#define shutdown(s,how)                           lwip_shutdown(s,how)
/** @ingroup socket */
#define getpeername(s,name,namelen)               lwip_getpeername(s,name,namelen)
/** @ingroup socket */
#define getsockname(s,name,namelen)               lwip_getsockname(s,name,namelen)
/** @ingroup socket */
#define setsockopt(s,level,optname,opval,optlen)  lwip_setsockopt(s,level,optname,opval,optlen)
/** @ingroup socket */
#define getsockopt(s,level,optname,opval,optlen)  lwip_getsockopt(s,level,optname,opval,optlen)
/** @ingroup socket */
#define closesocket(s)                            lwip_close(s)
/** @ingroup socket */
#define connect(s,name,namelen)                   lwip_connect(s,name,namelen)
/** @ingroup socket */
#define listen(s,backlog)                         lwip_listen(s,backlog)
/** @ingroup socket */
#define recv(s,mem,len,flags)                     lwip_recv(s,mem,len,flags)
/** @ingroup socket */
#define recvmsg(s,message,flags)                  lwip_recvmsg(s,message,flags)
/** @ingroup socket */
#define recvfrom(s,mem,len,flags,from,fromlen)    lwip_recvfrom(s,mem,len,flags,from,fromlen)
/** @ingroup socket */
#define send(s,dataptr,size,flags)                lwip_send(s,dataptr,size,flags)
/** @ingroup socket */
#define sendmsg(s,message,flags)                  lwip_sendmsg(s,message,flags)
/** @ingroup socket */
#define sendto(s,dataptr,size,flags,to,tolen)     lwip_sendto(s,dataptr,size,flags,to,tolen)
/** @ingroup socket */
#define socket(domain,type,protocol)              lwip_socket(domain,type,protocol)
#if LWIP_SOCKET_SELECT
/** @ingroup socket */
#define select(maxfdp1,readset,writeset,exceptset,timeout)     lwip_select(maxfdp1,readset,writeset,exceptset,timeout)
#endif
#if LWIP_SOCKET_POLL
/** @ingroup socket */
#define poll(fds,nfds,timeout)                    lwip_poll(fds,nfds,timeout)
#endif
/** @ingroup socket */
#define ioctlsocket(s,cmd,argp)                   lwip_ioctl(s,cmd,argp)
/** @ingroup socket */
#define inet_ntop(af,src,dst,size)                lwip_inet_ntop(af,src,dst,size)
/** @ingroup socket */
#define inet_pton(af,src,dst)                     lwip_inet_pton(af,src,dst)

#if LWIP_POSIX_SOCKETS_IO_NAMES
/** @ingroup socket */
#define read(s,mem,len)                           lwip_read(s,mem,len)
/** @ingroup socket */
#define readv(s,iov,iovcnt)                       lwip_readv(s,iov,iovcnt)
/** @ingroup socket */
#define write(s,dataptr,len)                      lwip_write(s,dataptr,len)
/** @ingroup socket */
#define writev(s,iov,iovcnt)                      lwip_writev(s,iov,iovcnt)
/** @ingroup socket */
#define close(s)                                  lwip_close(s)
/** @ingroup socket */
#define fcntl(s,cmd,val)                          lwip_fcntl(s,cmd,val)
/** @ingroup socket */
#define ioctl(s,cmd,argp)                         lwip_ioctl(s,cmd,argp)
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */
#endif /* LWIP_COMPAT_SOCKETS != 2 */

#endif /* LWIP_COMPAT_SOCKETS */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_SOCKET */

static inline esp_ip_t esp_ip_from_in_addr(const in_addr_t addr) {
  esp_ip_t espip;
  espip.ip[0] = addr & 0xff;
  espip.ip[1] = addr >> 8 & 0xff;
  espip.ip[2] = addr >> 16 & 0xff;
  espip.ip[3] = addr >> 24 & 0xff;
  return espip;
}

static inline esp_ip_t esp_ip_from_ip_addr(const ip_addr_t addr) {
  // TODO: This is not correct, what about ipv6 addrs
  return esp_ip_from_in_addr(addr.addr);
}

static inline in_addr_t addr_from_esp_ip(const esp_ip_t espip) {
  return *((in_addr_t*)espip.ip);
}

#endif /* LWESP_HDR_SOCKETS_H */
