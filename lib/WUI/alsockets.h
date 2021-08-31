#pragma once

#include "lwip/sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

struct alsockets_s;

void alsockets_funcs(struct alsockets_s *sockets_fncs);

int socket(int domain, int type, int protocol);
int listen(int s, int backlog);
int bind(int s, const struct sockaddr *name, socklen_t namelen);
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int connect(int s, const struct sockaddr *name, socklen_t namelen);
ssize_t read(int s, void *mem, size_t len);
ssize_t write(int s, const void *dataptr, size_t size);

ssize_t recvfrom(int s, void *mem, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen);
ssize_t sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);

int close(int s);

#ifdef __cplusplus
}
#endif
