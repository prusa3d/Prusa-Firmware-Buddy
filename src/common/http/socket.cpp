#include "socket.hpp"
#include <string.h>
#include <optional>

#if defined(__unix__) || defined(__APPLE__) || defined(__WIN32__)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <poll.h>
#else
    #include "sockets.h"
    #include "lwip/inet.h"
    #include "lwip/netdb.h"
#endif
#undef log_debug
#define log_debug(...)
#include <log.h>

#include <memory>

using std::unique_ptr;

LOG_COMPONENT_DEF(socket, LOG_SEVERITY_DEBUG);

namespace {

class AddrDeleter {
public:
    void operator()(addrinfo *addr) {
        freeaddrinfo(addr);
    }
};

}

namespace http {

socket_con::socket_con(uint8_t timeout_s)
    : Connection(timeout_s) {
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

std::optional<Error> socket_con::connection(const char *host, uint16_t port) {

    const struct timeval timeout = { get_timeout_s(), 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        return Error::SetSockOpt;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
        return Error::SetSockOpt;
    }

    int error;
    struct addrinfo hints;
    unique_ptr<addrinfo, AddrDeleter> addr_list;
    struct addrinfo *cur;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    constexpr size_t str_len = 6;
    char port_as_str[str_len] = {};
    snprintf(port_as_str, str_len, "%hu", port);

    if (getaddrinfo(host, port_as_str, &hints, &cur) != 0) {
        return Error::Dns;
    }

    addr_list.reset(cur);

    for (cur = addr_list.get(); cur != NULL; cur = cur->ai_next) {
        if (AF_INET == cur->ai_family) {
            error = ::connect(fd, cur->ai_addr, cur->ai_addrlen);
            if (0 == error) {
                connected = true;
                break;
            }
        }
    }

    if (!connected)
        return Error::Connect;
    else
        return std::nullopt;
}

std::variant<size_t, Error> socket_con::tx(const uint8_t *send_buffer, size_t data_len) {
    if (!connected)
        return Error::InternalError;

    size_t bytes_sent = 0;

    int status = send(fd, (const unsigned char *)send_buffer, data_len, 0);

    if (status < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return Error::Timeout;
        } else {
            return Error::Network;
        }
    }

    bytes_sent = (size_t)status;
    log_debug(socket, "-- written %d bytes --\n", bytes_sent);
    log_debug(socket, "%s", send_buffer);
    return bytes_sent;
}

std::variant<size_t, Error> socket_con::rx(uint8_t *read_buffer, size_t buffer_len, bool nonblock) {
    if (!connected)
        return Error::InternalError;

    size_t bytes_received = 0;

    int flags = nonblock ? MSG_DONTWAIT : 0;
    int status = recv(fd, (unsigned char *)read_buffer, buffer_len, flags);

    if (status < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return Error::Timeout;
        } else {
            return Error::Network;
        }
    }

    bytes_received = (size_t)status;
    log_debug(socket, "read %zu bytes\n", bytes_received);
    return bytes_received;
}

bool socket_con::poll_readable(uint32_t timeout) {
    pollfd descriptor = {};
    descriptor.fd = fd;
    descriptor.events = POLLIN;
    if (poll(&descriptor, 1, timeout) == 1) {
        return descriptor.revents & POLLIN;
    } else {
        return false;
    }
}

}
