/*
 * buddy_socket.c
 *
 *  Created on: Apr 1, 2021
 *      Author: joshy
 */
#include "buddy_socket.h"
#include "lan_interface.h"
#include "uart_socket.h"

static lan_interface_type interface_connected = BUDDY_LAN_ETH;

int socket(int domain, int type, int protocol) {

    interface_connected = get_lan_type();

    switch (interface_connected) {
    case BUDDY_LAN_ETH:
        lwip_socket(domain, type, protocol);
        break;
    case BUDDY_LAN_WIFI:
        uart_socket(domain, type, protocol);
        break;
    default:
        break;
    }
    return 0;
}

int connect(int s, const struct sockaddr *name, socklen_t namelen) {

    if (interface_connected != get_lan_type())
        return -1;

    switch (interface_connected) {
    case BUDDY_LAN_ETH:
        lwip_connect(s, name, namelen);
        break;
    case BUDDY_LAN_WIFI:
        uart_connect(s, name, namelen);
        break;
    default:
        break;
    }
    return 0;
}

ssize_t send(int s, const void *data, size_t size, int flags) {
    int ret = -1;

    if (interface_connected != get_lan_type())
        return -1;

    switch (interface_connected) {
    case BUDDY_LAN_ETH:
        ret = lwip_send(s, data, size, 0);
        break;
    case BUDDY_LAN_WIFI:
        ret = uart_send(s, data, size, 0);
        break;
    default:
        break;
    }
    return ret;
}

ssize_t recv(int s, void *mem, size_t len, int flags) {
    ssize_t rec_len = -1;

    if (interface_connected != get_lan_type())
        return -1;

    switch (interface_connected) {
    case BUDDY_LAN_ETH:
        rec_len = lwip_recv(s, mem, len, flags);
        break;
    case BUDDY_LAN_WIFI:
        rec_len = uart_recv(s, mem, len, flags);
        break;
    default:
        break;
    }
    return rec_len;
}

int close(int s) {
    int ret = -1;

    if (interface_connected != get_lan_type())
        return -1;

    switch (interface_connected) {
    case BUDDY_LAN_ETH:
        ret = lwip_close(s);
        break;
    case BUDDY_LAN_WIFI:
        ret = uart_close(s);
        break;
    default:
        break;
    }
    return ret;
}
