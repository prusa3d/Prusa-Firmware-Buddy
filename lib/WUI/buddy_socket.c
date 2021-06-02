/*
 * buddy_socket.c
 *
 *  Created on: Apr 1, 2021
 *      Author: joshy
 */
#include "buddy_socket.h"
#include "lan_interface.h"
#include "cmsis_os.h"

#define SOCKET_ARRAY_SIZE 8

static uint8_t socket_array_reset = 1;
static socket_t socket_array[SOCKET_ARRAY_SIZE];

socket_t *socket_init() {
    socket_t *s = NULL;
    // initialize socket array at first use
    if (socket_array_reset) {
        for (int i = 0; i < SOCKET_ARRAY_SIZE; i++) {
            socket_array[i].buddy_f_d = -1;
            socket_array[i].file_descriptor = -1;
            socket_array[i].interface = BUDDY_LAN_ETH;
        }
        socket_array_reset = 0;
    }
    // allocate fist available socket
    for (int j = 0; j < SOCKET_ARRAY_SIZE; j++) {
        if (-1 == socket_array[j].buddy_f_d) {
            socket_array[j].buddy_f_d = j;
            socket_array[j].interface = get_lan_type();
            s = &socket_array[j];
            break;
        }
    }
    return s;
}

void socket_deinit(uint8_t id) {
    if ((id < 0) || (id >= SOCKET_ARRAY_SIZE)) {
        return;
    }
    socket_array[id].buddy_f_d = -1;
    socket_array[id].file_descriptor = -1;
    socket_array[id].interface = -1;
}

socket_t *get_socket_from_id(uint8_t id) {
    if ((id >= 0) && (id < SOCKET_ARRAY_SIZE)) {
        if (socket_array[id].buddy_f_d != -1)
            return &socket_array[id];
        else
            return NULL;
    } else
        return NULL;
}

int socket(int domain, int type, int protocol) {

    socket_t *s = socket_init();
    if (NULL == s) {
        return -1;
    }

    switch (s->interface) {
    case BUDDY_LAN_ETH:
        s->file_descriptor = lwip_socket(domain, type, protocol);
        if (LWIP_SOCKET_ERROR == s->file_descriptor) {
            socket_deinit(s->buddy_f_d);
        } else {
            return s->buddy_f_d;
        }
        break;
    case BUDDY_LAN_WIFI:
    default:
        return -1;
        break;
    }

    return -1;
}

int connect(int s, const struct sockaddr *name, socklen_t namelen) {

    if (socket_array[s].interface != get_lan_type())
        return -1;

    switch (socket_array[s].interface) {
    case BUDDY_LAN_ETH:
        return lwip_connect(socket_array[s].file_descriptor, name, namelen);
        break;
    case BUDDY_LAN_WIFI:
    default:
        return -1;
        break;
    }
    return 0;
}

ssize_t send(int s, const void *data, size_t size, int flags) {
    int ret = -1;
    int sent_byte = 0;

    if (socket_array[s].interface != get_lan_type())
        return -1;

    switch (socket_array[s].interface) {
    case BUDDY_LAN_ETH:
        sent_byte = lwip_send(socket_array[s].file_descriptor, data, size, 0);
        if (sent_byte > 0)
            ret = 0;
        break;
    case BUDDY_LAN_WIFI:
    default:
        return -1;
        break;
    }
    osDelay(100);
    return ret;
}

int sendto(int s, void *data, size_t size, int flags, struct sockaddr *addr,
    socklen_t length) {
    int ret = -1;
    int sent_byte = 0;

    if (socket_array[s].interface != get_lan_type())
        return -1;

    switch (socket_array[s].interface) {
    case BUDDY_LAN_ETH:
        sent_byte = lwip_sendto(socket_array[s].file_descriptor, data, size, flags, addr, length);
        if (sent_byte > 0)
            ret = 0;
        break;
    case BUDDY_LAN_WIFI:
    default:
        ret = -1;
        break;
    }
    return ret;
}
ssize_t recv(int s, void *mem, size_t len, int flags) {
    ssize_t rec_len = -1;

    if (socket_array[s].interface != get_lan_type())
        return -1;

    switch (socket_array[s].interface) {
    case BUDDY_LAN_ETH:
        rec_len = lwip_recv(socket_array[s].file_descriptor, mem, len, flags);
        break;
    case BUDDY_LAN_WIFI:
    default:
        return -1;
        break;
    }
    return rec_len;
}

int close(int s) {
    int ret = -1;

    switch (socket_array[s].interface) {
    case BUDDY_LAN_ETH:
        ret = lwip_close(socket_array[s].file_descriptor);
        socket_deinit(s);
        break;
    case BUDDY_LAN_WIFI:
    default:
        return -1;
        break;
    }
    return ret;
}

int bind(int s, const struct sockaddr *name, socklen_t namelen) {
    int ret = lwip_bind(socket_array[s].file_descriptor, name, namelen);
    return ret;
}

int listen(int s, int backlog) {
    int ret = lwip_listen(socket_array[s].file_descriptor, backlog);
    return ret;
}

ssize_t read(int s, void *mem, size_t len) {
    return lwip_read(socket_array[s].file_descriptor, mem, len);
}

ssize_t write(int s, const void *data, size_t size) {
    return lwip_write(socket_array[s].file_descriptor, data, size);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    socket_t *sock = socket_init();
    if (NULL != sock) {
        sock->file_descriptor = lwip_accept(socket_array[s].file_descriptor, addr, addrlen);
        if (-1 != sock->file_descriptor)
            return sock->buddy_f_d;
        else
            socket_deinit(sock->buddy_f_d);
    }
    return -1;
}

error_t socketSetTimeout(socket_t *socket, systime_t timeout) {
    return NO_ERROR;
}
