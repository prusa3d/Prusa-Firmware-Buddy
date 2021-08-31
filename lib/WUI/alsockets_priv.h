#pragma once

#include "alsockets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*alsockets_socket_fn)(int domain, int type, int protocol);
typedef int (*alsockets_listen_fn)(int s, int backlog);
typedef int (*alsockets_bind_fn)(int s, const struct sockaddr *name, socklen_t namelen);
typedef int (*alsockets_accept_fn)(int s, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*alsockets_connect_fn)(int s, const struct sockaddr *name, socklen_t namelen);
typedef ssize_t (*alsockets_read_fn)(int s, void *mem, size_t len);
typedef ssize_t (*alsockets_write_fn)(int s, const void *dataptr, size_t size);
typedef int (*alsockets_close_fn)(int s);

typedef ssize_t (*alsockets_recvfrom_fn)(int s, void *mem, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen);
typedef ssize_t (*alsockets_sendto_fn)(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);

typedef struct alsockets_s {
    alsockets_socket_fn socket;
    alsockets_listen_fn listen;
    alsockets_bind_fn bind;
    alsockets_accept_fn accept;
    alsockets_connect_fn connect;
    alsockets_read_fn read;
    alsockets_write_fn write;
    alsockets_close_fn close;
    alsockets_recvfrom_fn recvfrom;
    alsockets_sendto_fn sendto;
} alsockets_t;

#ifdef __cplusplus
}
#endif
