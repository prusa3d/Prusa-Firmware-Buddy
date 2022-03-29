#include "socket.hpp"
#include <string.h>
#include <optional>
#include <debug.h>

#ifdef __unix__
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
#else
    #include "sockets.h"
    #include "lwip/inet.h"
    #include "lwip/netdb.h"
#endif
#undef log_debug
#define log_debug(...)

LOG_COMPONENT_DEF(socket, LOG_SEVERITY_DEBUG);

namespace con {

socket_con::socket_con() {
    fd = -1;
    connected = false;
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        log_debug(socket, "%s", "socket creation failed\n");
    }
    log_debug(socket, "socket created with fd: %d\n", fd);
}

socket_con::~socket_con() {
    log_debug(socket, "socket destructor called: %d\n", fd);
    if (-1 != fd) {
        log_debug(socket, "shutting down socket: %d\n", fd);
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }
}

std::optional<Error> socket_con::connection(char *host, uint16_t port) {

    const struct timeval timeout = { SOCKET_TIMEOUT_SEC, 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, NULL, 0);

    int error;
    struct addrinfo hints;
    struct addrinfo *addr_list;
    struct addrinfo *cur;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    constexpr size_t str_len = 6;
    char port_as_str[str_len] = {};
    snprintf(port_as_str, str_len, "%hu", port);

    if (getaddrinfo(host, port_as_str, &hints, &addr_list) != 0) {
        return Error::CONNECTION_ERROR;
    }

    for (cur = addr_list; cur != NULL; cur = cur->ai_next) {
        if (AF_INET == cur->ai_family) {
            error = ::connect(fd, cur->ai_addr, cur->ai_addrlen);
            if (0 == error) {
                connected = true;
                break;
            }
        }
    }
    freeaddrinfo(addr_list);

    if (!connected)
        return Error::CONNECTION_ERROR;
    else
        return std::nullopt;
}

std::variant<size_t, Error> socket_con::tx(uint8_t *send_buffer, size_t data_len) {
    if (!connected)
        return Error::WRITE_ERROR;

    size_t bytes_sent = 0;

    int status = ::write(fd, (const unsigned char *)send_buffer, data_len);

    if (status < 0) {
        return Error::WRITE_ERROR;
    }

    bytes_sent = (size_t)status;
    log_debug(socket, "-- written %d bytes --\n", bytes_sent);
    log_debug(socket, "%s", send_buffer);
    return bytes_sent;
}

std::variant<size_t, Error> socket_con::rx(uint8_t *read_buffer, size_t buffer_len) {
    if (!connected)
        return Error::WRITE_ERROR;

    size_t bytes_received = 0;

    int status = ::read(fd, (unsigned char *)read_buffer, buffer_len);

    if (status < 0) {
        return Error::READ_ERROR;
    }

    bytes_received = (size_t)status;
    CONNECT_DEBUG("read %zu bytes\n", bytes_received);
    return bytes_received;
}

}
