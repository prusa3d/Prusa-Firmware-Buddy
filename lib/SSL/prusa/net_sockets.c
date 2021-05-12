/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name       : net_sockets.c.h
  * Description     : TCP/IP or UDP/IP networking functions implementation based
                    on LwIP API see the file "mbedTLS/library/net_socket_template.c"
                    for the standard implmentation
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/*
 * This is a template implmentation of the net_socket.c based on the LwIP
 * TCP/IP Stack.
 *
 */
/* USER CODE END Header */

#if !defined(MBEDTLS_CONFIG_FILE)
    #include "mbedtls/config.h"
#else
    #include MBEDTLS_CONFIG_FILE
#endif

#include <string.h>
#include <stdint.h>
#if defined(MBEDTLS_NET_C)

    #if defined(MBEDTLS_PLATFORM_C)
        #include "mbedtls/platform.h"
    #else
        #include <stdlib.h>
    #endif

    #include "mbedtls/net_sockets.h"

/*
 * LwIP header files
 * make sure that the LwIP project config file, "lwipopts.h", is enabling the following flags
 * LWIP_TCP==1            : Enable TCP
 * LWIP_UDP==1            : Enable UDP
 * LWIP_DNS==1            : Enable DNS module (could be optional depending on the application)
 * LWIP_SOCKET==1         : Enable Socket API
 * LWIP_COMPAT_SOCKETS==1 : Enable BSD-style sockets functions
 * SO_REUSE==1            : Enable SO_REUSEADDR option
 */

    #include "lwip/netdb.h"
    #include "lwip/sockets.h"

//#include "netif/ethernet.h"

struct sockaddr_storage client_addr;

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

static int net_would_block(const mbedtls_net_context *ctx);
/* USER CODE BEGIN VARIABLES */

/* USER CODE END VARIABLES */
/*
 * Initialize LwIP stack
 */
void mbedtls_net_init(mbedtls_net_context *ctx) {
    /* USER CODE BEGIN 0 */

    /* USER CODE END 0 */
    //  MX_LWIP_Init();
    /* USER CODE BEGIN 1 */
    ctx->fd = -1;
    /* USER CODE END 1 */
}

/*
 * Initiate a TCP connection with host:port and the given protocol
 */
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int proto) {
    int ret;
    struct addrinfo hints;
    struct addrinfo *addr_list;
    struct addrinfo *cur;
    /* USER CODE BEGIN 2 */

    /* USER CODE END 2 */

    /* Do name resolution with both IPv6 and IPv4 */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;
    /* USER CODE BEGIN 3 */

    /* USER CODE END 3 */

    if (getaddrinfo(host, port, &hints, &addr_list) != 0)
        return (MBEDTLS_ERR_NET_UNKNOWN_HOST);
    /* USER CODE BEGIN 4 */

    /* USER CODE END 4 */

    /* Try the sockaddrs until a connection succeeds */
    ret = MBEDTLS_ERR_NET_UNKNOWN_HOST;
    for (cur = addr_list; cur != NULL; cur = cur->ai_next) {
        ctx->fd = (int)socket(cur->ai_family, cur->ai_socktype,
            cur->ai_protocol);
        /* USER CODE BEGIN 5 */

        /* USER CODE END 5 */
        if (ctx->fd < 0) {
            ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
            continue;
        }
        /* USER CODE BEGIN 6 */

        /* USER CODE END 6 */

        if (connect(ctx->fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            ret = 0;
            break;
        }
        /* USER CODE BEGIN 7 */

        /* USER CODE END 7 */

        close(ctx->fd);
        /* USER CODE BEGIN 8 */

        /* USER CODE END 8 */
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    }
    /* USER CODE BEGIN 9 */

    /* USER CODE END 9 */

    freeaddrinfo(addr_list);

    return (ret);
}

/*
 * Create a listening socket on bind_ip:port
 */
int mbedtls_net_bind(mbedtls_net_context *ctx, const char *bind_ip, const char *port, int proto) {
    int n, ret;
    struct addrinfo hints, *addr_list, *cur;

    /* Bind to IPv6 and/or IPv4, but only in the desired protocol */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;
    if (bind_ip == NULL)
        hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(bind_ip, port, &hints, &addr_list) != 0)
        return (MBEDTLS_ERR_NET_UNKNOWN_HOST);

    /* Try the sockaddrs until a binding succeeds */
    ret = MBEDTLS_ERR_NET_UNKNOWN_HOST;
    for (cur = addr_list; cur != NULL; cur = cur->ai_next) {
        ctx->fd = (int)socket(cur->ai_family, cur->ai_socktype,
            cur->ai_protocol);
        if (ctx->fd < 0) {
            ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
            continue;
        }

        n = 1;
        if (setsockopt(ctx->fd, SOL_SOCKET, SO_REUSEADDR,
                (const char *)&n, sizeof(n))
            != 0) {
            close(ctx->fd);
            ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
            continue;
        }

        if (bind(ctx->fd, cur->ai_addr, cur->ai_addrlen) != 0) {
            close(ctx->fd);
            ret = MBEDTLS_ERR_NET_BIND_FAILED;
            continue;
        }

        /* Listen only makes sense for TCP */
        if (proto == MBEDTLS_NET_PROTO_TCP) {
            if (listen(ctx->fd, MBEDTLS_NET_LISTEN_BACKLOG) != 0) {
                close(ctx->fd);
                ret = MBEDTLS_ERR_NET_LISTEN_FAILED;
                continue;
            }
        }

        /* Bind was successful */
        ret = 0;
        break;
    }

    freeaddrinfo(addr_list);

    return (ret);
}

/*
 * Check if the requested operation would be blocking on a non-blocking socket
 * and thus 'failed' with a negative return value.
 *
 * Note: on a blocking socket this function always returns 0!
 */
static int net_would_block(const mbedtls_net_context *ctx) {
    int err = errno;

    /*
     * Never return 'WOULD BLOCK' on a non-blocking socket
     */
    if (fcntl(ctx->fd, F_GETFL, O_NONBLOCK) != O_NONBLOCK) {
        errno = err;
        return (0);
    }

    switch (errno = err) {
    #if defined EAGAIN
    case EAGAIN:
    #endif
    #if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
    #endif
        return (1);
    }
    return (0);
}

/*
 * Accept a connection from a remote client
 */
int mbedtls_net_accept(mbedtls_net_context *bind_ctx,
    mbedtls_net_context *client_ctx,
    void *client_ip, size_t buf_size, size_t *ip_len) {
    int ret;
    int type;

    struct sockaddr_storage client_addr;

    socklen_t n = (socklen_t)sizeof(client_addr);
    socklen_t type_len = (socklen_t)sizeof(type);

    /* Is this a TCP or UDP socket? */
    if (getsockopt(bind_ctx->fd, SOL_SOCKET, SO_TYPE,
            (void *)&type, &type_len)
            != 0
        || (type != SOCK_STREAM && type != SOCK_DGRAM)) {
        return (MBEDTLS_ERR_NET_ACCEPT_FAILED);
    }

    if (type == SOCK_STREAM) {
        /* TCP: actual accept() */
        ret = client_ctx->fd = (int)accept(bind_ctx->fd,
            (struct sockaddr *)&client_addr, &n);
    } else {
        /* UDP: wait for a message, but keep it in the queue */
        char buf[1] = { 0 };

        ret = (int)recvfrom(bind_ctx->fd, buf, sizeof(buf), MSG_PEEK,
            (struct sockaddr *)&client_addr, &n);
    }

    if (ret < 0) {
        if (net_would_block(bind_ctx) != 0)
            return (MBEDTLS_ERR_SSL_WANT_READ);

        return (MBEDTLS_ERR_NET_ACCEPT_FAILED);
    }

    /* UDP: hijack the listening socket to communicate with the client,
     * then bind a new socket to accept new connections */
    if (type != SOCK_STREAM) {
        struct sockaddr_storage local_addr;
        int one = 1;

        if (connect(bind_ctx->fd, (struct sockaddr *)&client_addr, n) != 0)
            return (MBEDTLS_ERR_NET_ACCEPT_FAILED);

        client_ctx->fd = bind_ctx->fd;
        bind_ctx->fd = -1; /* In case we exit early */

        n = sizeof(struct sockaddr_storage);
        if (getsockname(client_ctx->fd,
                (struct sockaddr *)&local_addr, &n)
                != 0
            || (bind_ctx->fd = (int)socket(local_addr.ss_family,
                    SOCK_DGRAM, IPPROTO_UDP))
                < 0
            || setsockopt(bind_ctx->fd, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&one, sizeof(one))
                != 0) {
            return (MBEDTLS_ERR_NET_SOCKET_FAILED);
        }

        if (bind(bind_ctx->fd, (struct sockaddr *)&local_addr, n) != 0) {
            return (MBEDTLS_ERR_NET_BIND_FAILED);
        }
    }

    if (client_ip != NULL) {
        if (client_addr.ss_family == AF_INET) {
    #if LWIP_IPV4
            struct sockaddr_in *addr4 = (struct sockaddr_in *)&client_addr;
            *ip_len = sizeof(addr4->sin_addr.s_addr);

            if (buf_size < *ip_len)
                return (MBEDTLS_ERR_NET_BUFFER_TOO_SMALL);

            memcpy(client_ip, &addr4->sin_addr.s_addr, *ip_len);
    #endif
        } else {
    #if LWIP_IPV6
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&client_addr;
            *ip_len = sizeof(addr6->sin6_addr.s6_addr);

            if (buf_size < *ip_len)
                return (MBEDTLS_ERR_NET_BUFFER_TOO_SMALL);

            memcpy(client_ip, &addr6->sin6_addr.s6_addr, *ip_len);
    #endif
        }
    }

    return (0);
}

/*
 * Set the socket blocking or non-blocking
 */
int mbedtls_net_set_block(mbedtls_net_context *ctx) {
    /* LwIP doesn't currently support it  */
    return (1);
}

int mbedtls_net_set_nonblock(mbedtls_net_context *ctx) {

    return (fcntl(ctx->fd, F_SETFL, fcntl(ctx->fd, F_GETFL, 0) | O_NONBLOCK));
}

/*
 * Check if data is available on the socket
 */

int mbedtls_net_poll(mbedtls_net_context *ctx, uint32_t rw, uint32_t timeout) {
    int ret;
    struct timeval tv;

    fd_set read_fds;
    fd_set write_fds;

    int fd = ctx->fd;

    if (fd < 0)
        return (MBEDTLS_ERR_NET_INVALID_CONTEXT);

    FD_ZERO(&read_fds);
    if (rw & MBEDTLS_NET_POLL_READ) {
        rw &= ~MBEDTLS_NET_POLL_READ;
        FD_SET(fd, &read_fds);
    }

    FD_ZERO(&write_fds);
    if (rw & MBEDTLS_NET_POLL_WRITE) {
        rw &= ~MBEDTLS_NET_POLL_WRITE;
        FD_SET(fd, &write_fds);
    }

    if (rw != 0)
        return (MBEDTLS_ERR_NET_BAD_INPUT_DATA);

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    do {
        ret = select(fd + 1, &read_fds, &write_fds, NULL,
            timeout == (uint32_t)-1 ? NULL : &tv);
    } while (ret == EINTR);

    if (ret < 0)
        return (MBEDTLS_ERR_NET_POLL_FAILED);

    ret = 0;
    if (FD_ISSET(fd, &read_fds))
        ret |= MBEDTLS_NET_POLL_READ;
    if (FD_ISSET(fd, &write_fds))
        ret |= MBEDTLS_NET_POLL_WRITE;

    return (ret);
}

/*
 * Portable usleep helper
 */
void mbedtls_net_usleep(unsigned long usec) {
    struct timeval tv;

    tv.tv_sec = usec / 1000000;
    tv.tv_usec = usec % 1000000;

    select(0, NULL, NULL, NULL, &tv);
}

/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
    int ret;
    int fd = ((mbedtls_net_context *)ctx)->fd;

    if (fd < 0)
        return (MBEDTLS_ERR_NET_INVALID_CONTEXT);

    ret = (int)read(fd, buf, len);

    if (ret < 0) {
        if (net_would_block(ctx) != 0)
            return (MBEDTLS_ERR_SSL_WANT_READ);

        if (errno == EPIPE || errno == ECONNRESET)
            return (MBEDTLS_ERR_NET_CONN_RESET);

        if (errno == EINTR)
            return (MBEDTLS_ERR_SSL_WANT_READ);
        /* USER CODE BEGIN 15 */

        /* USER CODE END 15 */
        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }

    return (ret);
}

/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
 */
int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf,
    size_t len, uint32_t timeout) {
    int ret;
    struct timeval tv;
    fd_set read_fds;
    int fd = ((mbedtls_net_context *)ctx)->fd;

    if (fd < 0)
        return (MBEDTLS_ERR_NET_INVALID_CONTEXT);

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    ret = select(fd + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);

    /* Zero fds ready means we timed out */
    if (ret == 0)
        return (MBEDTLS_ERR_SSL_TIMEOUT);

    if (ret < 0) {

        if (errno == EINTR)
            return (MBEDTLS_ERR_SSL_WANT_READ);

        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }

    /* This call will not block */
    return (mbedtls_net_recv(ctx, buf, len));
}

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len) {
    int ret;
    int fd = ((mbedtls_net_context *)ctx)->fd;

    if (fd < 0)
        return (MBEDTLS_ERR_NET_INVALID_CONTEXT);

    ret = (int)write(fd, buf, len);

    if (ret < 0) {
        if (net_would_block(ctx) != 0)
            return (MBEDTLS_ERR_SSL_WANT_WRITE);

        if (errno == EPIPE || errno == ECONNRESET)
            return (MBEDTLS_ERR_NET_CONN_RESET);

        if (errno == EINTR)
            return (MBEDTLS_ERR_SSL_WANT_WRITE);
        /* USER CODE BEGIN 17 */

        /* USER CODE END 17 */
        return (MBEDTLS_ERR_NET_SEND_FAILED);
    }

    return (ret);
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free(mbedtls_net_context *ctx) {
    if (ctx->fd == -1)
        return;
    /* USER CODE BEGIN 18 */

    /* USER CODE END 18 */
    shutdown(ctx->fd, 2);
    close(ctx->fd);

    ctx->fd = -1;
}

#endif /* MBEDTLS_NET_C */
