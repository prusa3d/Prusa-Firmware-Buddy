/*
 * buddy_socket.h
 *
 *  Created on: Apr 1, 2021
 *      Author: joshy
 */

#pragma once

#include "lwip/sockets.h"
#include "lan_interface.h"
#include "os_port.h"
#include "error.h"

#define LWIP_SOCKET_ERROR (-1)

struct sockaddr_in address_httpd, remotehost;

typedef struct {
    int file_descriptor;
    int buddy_f_d;
    lan_interface_type interface;
} socket_t;

// CycloneTCP specific thing
typedef enum {
    SOCKET_FLAG_PEEK = 0x0200,
    SOCKET_FLAG_DONT_ROUTE = 0x0400,
    SOCKET_FLAG_WAIT_ALL = 0x0800,
    SOCKET_FLAG_DONT_WAIT = 0x0100,
    SOCKET_FLAG_BREAK_CHAR = 0x1000,
    SOCKET_FLAG_BREAK_CRLF = 0x100A,
    SOCKET_FLAG_WAIT_ACK = 0x2000,
    SOCKET_FLAG_NO_DELAY = 0x4000,
    SOCKET_FLAG_DELAY = 0x8000
} SocketFlags;

socket_t *socket_init();

socket_t *get_socket_from_id(uint8_t id);

int socket(int domain, int type, int protocol);

int bind(int s, const struct sockaddr *name, socklen_t namelen);

int listen(int s, int backlog);

int accept(int s, struct sockaddr *addr, socklen_t *addrlen);

int connect(int s, const struct sockaddr *name, socklen_t namelen);

ssize_t send(int s, const void *data, size_t size, int flags);

int sendto(int socket, void *buffer, size_t size, int flags, struct sockaddr *addr,
    socklen_t length);

ssize_t recv(int s, void *mem, size_t len, int flags);

ssize_t read();

ssize_t write();

int close(int s);

// stub function
error_t socketSetTimeout(socket_t *socket, systime_t timeout);
