/*
 * buddy_socket.h
 *
 *  Created on: Apr 1, 2021
 *      Author: joshy
 */

#pragma once

#include "lwip/sockets.h"

int socket(int domain, int type, int protocol);

int connect(int s, const struct sockaddr *name, socklen_t namelen);

ssize_t send(int s, const void *data, size_t size, int flags);

ssize_t recv(int s, void *mem, size_t len, int flags);

int sendto(int socket, void *buffer, size_t size, int flags, struct sockaddr *addr,
    socklen_t length);

int close(int s);
