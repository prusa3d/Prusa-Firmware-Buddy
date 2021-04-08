/*
 * buddy_socket.c
 *
 *  Created on: Apr 1, 2021
 *      Author: joshy
 */
#include "buddy_socket.h"
#include "uart_socket.h"

typedef enum {
    BUDDY_LINK_ETH = 0,
    BUDDY_LINK_WIFI,
} buddy_link_type;

static buddy_link_type buddy_link = BUDDY_LINK_WIFI;

int buddy_link_type_set(buddy_link_type type) {
    int err = 0;
    switch (type) {
    case BUDDY_LINK_ETH:
    case BUDDY_LINK_WIFI:
        buddy_link = type;
        break;
    default:
        err = -1;
        break;
    }
    return err;
}

int socket(int domain, int type, int protocol) {
    switch (buddy_link) {
    case BUDDY_LINK_ETH:
        lwip_socket(domain, type, protocol);
        break;
    case BUDDY_LINK_WIFI:
        uart_socket(domain, type, protocol);
        break;
    default:
        break;
    }
    return 0;
}

int connect(int s, const struct sockaddr *name, socklen_t namelen) {
    switch (buddy_link) {
    case BUDDY_LINK_ETH:
        lwip_connect(s, name, namelen);
        break;
    case BUDDY_LINK_WIFI:
        uart_connect(s, name, namelen);
        break;
    default:
        break;
    }
    return 0;
}

ssize_t send(int s, const void *data, size_t size, int flags) {
    int ret = -1;
    switch (buddy_link) {
    case BUDDY_LINK_ETH:
        ret = lwip_send(s, data, size, 0);
        break;
    case BUDDY_LINK_WIFI:
        ret = uart_send(s, data, size, 0);
        break;
    default:
        break;
    }
    return ret;
}

ssize_t recv(int s, void *mem, size_t len, int flags) {
    ssize_t rec_len = -1;
    switch (buddy_link) {
    case BUDDY_LINK_ETH:
        rec_len = lwip_recv(s, mem, len, flags);
        break;
    case BUDDY_LINK_WIFI:
        rec_len = uart_recv(s, mem, len, flags);
        break;
    default:
        break;
    }
    return rec_len;
}

int close(int s) {
    int ret = -1;
    switch (buddy_link) {
    case BUDDY_LINK_ETH:
        ret = lwip_close(s);
        break;
    case BUDDY_LINK_WIFI:
        ret = uart_close(s);
        break;
    default:
        break;
    }
    return ret;
}
