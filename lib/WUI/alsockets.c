#include "alsockets.h"

#include "alsockets_priv.h"
#include "lwip/sockets.h"

static alsockets_t *alsockets;

void alsockets_funcs(struct alsockets_s *sockets_fncs) {
    alsockets = sockets_fncs;
}

int socket(int domain, int type, int protocol) {
    if (alsockets) {
        return alsockets->socket(domain, type, protocol);
    } else {
        return -1;
    }
}

int listen(int s, int backlog) {
    if (alsockets) {
        return alsockets->listen(s, backlog);
    } else {
        return -1;
    }
}

int bind(int s, const struct sockaddr *name, socklen_t namelen) {
    if (alsockets) {
        return alsockets->bind(s, name, namelen);
    } else {
        return -1;
    }
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    if (alsockets) {
        return alsockets->accept(s, addr, addrlen);
    } else {
        return -1;
    }
}

int connect(int s, const struct sockaddr *name, socklen_t namelen) {
    if (alsockets) {
        return alsockets->connect(s, name, namelen);
    } else {
        return -1;
    }
}

ssize_t read(int s, void *mem, size_t len) {
    if (alsockets) {
        return alsockets->read(s, mem, len);
    } else {
        return -1;
    }
}

ssize_t write(int s, const void *dataptr, size_t size) {
    if (alsockets) {
        return alsockets->write(s, dataptr, size);
    } else {
        return -1;
    }
}

int close(int s) {
    if (alsockets) {
        return alsockets->close(s);
    } else {
        return -1;
    }
}

ssize_t recvfrom(int s, void *mem, size_t len, int flags,
    struct sockaddr *from, socklen_t *fromlen) {
    if (alsockets) {
        return alsockets->recvfrom(s, mem, len, flags, from, fromlen);
    } else {
        return -1;
    }
}

ssize_t sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen) {
    if (alsockets) {
        return alsockets->sendto(s, dataptr, size, flags, to, tolen);
    } else {
        return -1;
    }
}

const struct alsockets_s eth_sockets = {
    .socket = lwip_socket,
    .listen = lwip_listen,
    .bind = lwip_bind,
    .accept = lwip_accept,
    .connect = lwip_connect,
    .read = lwip_read,
    .write = lwip_write,
    .close = lwip_close,
    .recvfrom = lwip_recvfrom,
    .sendto = lwip_sendto
};

const struct alsockets_s *alsockets_eth() {
    return &eth_sockets;
}
