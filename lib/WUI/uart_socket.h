/*
 * sockets_uart.h
 *
 *  Created on: Apr 1, 2021
 *      Author: joshy
 */

#pragma once

#include "lwip/sockets.h"

void configure_uart_socket(uint32_t baudrate);

int uart_socket(int domain, int type, int protocol);

int uart_connect(int s, const struct sockaddr *name, socklen_t namelen);

ssize_t uart_send(int s, const void *data, size_t size, int flags);

ssize_t uart_recv(int s, void *mem, size_t len, int flags);

int uart_close(int s);
