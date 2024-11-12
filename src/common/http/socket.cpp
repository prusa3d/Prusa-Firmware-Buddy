#include "socket.hpp"
#include <string.h>
#include <optional>

#if defined(__unix__) || defined(__APPLE__) || defined(__WIN32__)
// TODO: Clean up this hack for unit test sake...
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

    #define lwip_close        close
    #define lwip_connect      connect
    #define lwip_freeaddrinfo freeaddrinfo
    #define lwip_getaddrinfo  getaddrinfo
    #define lwip_poll         poll
    #define lwip_recv         recv
    #define lwip_send         send
    #define lwip_setsockopt   setsockopt
    #define lwip_shutdown     shutdown
    #define lwip_socket       socket
#else
    #include "sockets.h"
    #include "lwip/inet.h"
    #include "lwip/netdb.h"
#endif
#include <logging/log.hpp>

#include <memory>

using std::unique_ptr;

LOG_COMPONENT_DEF(socket, logging::Severity::info);

namespace {

class AddrDeleter {
public:
    void operator()(addrinfo *addr) {
        lwip_freeaddrinfo(addr);
    }
};

} // namespace

namespace http {

socket_con::socket_con(uint8_t timeout_s)
    : Connection(timeout_s) {
    fd = -1;
    connected = false;
    if ((fd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        log_debug(socket, "%s", "socket creation failed\n");
    }
    log_debug(socket, "socket created with fd: %d\n", fd);
}

socket_con::~socket_con() {
    log_debug(socket, "socket destructor called: %d\n", fd);
    if (-1 != fd) {
        log_debug(socket, "shutting down socket: %d\n", fd);
        lwip_close(fd);
    }
}

std::optional<Error> socket_con::connection(const char *host, uint16_t port) {

    const struct timeval timeout = { get_timeout_s(), 0 };
    if (lwip_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        return Error::SetSockOpt;
    }
    if (lwip_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
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

    if (lwip_getaddrinfo(host, port_as_str, &hints, &cur) != 0) {
        log_info(socket, "DNS resolution failed on host: %s", host);
        return Error::Dns;
    }

    addr_list.reset(cur);

    for (cur = addr_list.get(); cur != NULL; cur = cur->ai_next) {
        if (AF_INET == cur->ai_family) {
            error = lwip_connect(fd, cur->ai_addr, cur->ai_addrlen);
            if (0 == error) {
                connected = true;
                break;
            }
        }
    }

    if (!connected) {
        return Error::Connect;
    } else {
        return std::nullopt;
    }
}

std::variant<size_t, Error> socket_con::tx(const uint8_t *send_buffer, size_t data_len) {
    if (!connected) {
        return Error::InternalError;
    }

    size_t bytes_sent = 0;

    int status = lwip_send(fd, (const unsigned char *)send_buffer, data_len, 0);

    if (status < 0) {
        // Store errno, as the log_error may replace it.
        int e = errno;
        log_error(socket, "lwip send failed with: %d, errno: %d", status, e);
#pragma GCC diagnostic push
        // On some systems, EWOULDBLOCK and EAGAIN have the same value (and produce a warning), on some others they are different and need checking for both.
#pragma GCC diagnostic ignored "-Wlogical-op"
        if (e == EWOULDBLOCK || e == EAGAIN) {
            return Error::Timeout;
        } else {
            return Error::Network;
        }
#pragma GCC diagnostic pop
    }

    bytes_sent = (size_t)status;
    log_debug(socket, "-- written %d bytes --\n", bytes_sent);
    log_debug(socket, "%s", send_buffer);
    return bytes_sent;
}

std::variant<size_t, Error> socket_con::rx(uint8_t *read_buffer, size_t buffer_len, bool nonblock) {
    if (!connected) {
        return Error::InternalError;
    }

    size_t bytes_received = 0;

    int flags = nonblock ? MSG_DONTWAIT : 0;
    int status = lwip_recv(fd, (unsigned char *)read_buffer, buffer_len, flags);

    if (status < 0) {
        // Store errno, as the log_error may replace it.
        int e = errno;
        log_error(socket, "lwip recv failed with: %d, errno: %d", status, e);
#pragma GCC diagnostic push
        // On some systems, EWOULDBLOCK and EAGAIN have the same value (and produce a warning), on some others they are different and need checking for both.
#pragma GCC diagnostic ignored "-Wlogical-op"
        if (e == EWOULDBLOCK || e == EAGAIN) {
            return Error::Timeout;
        } else {
            return Error::Network;
        }
#pragma GCC diagnostic pop
    }

    bytes_received = (size_t)status;
    log_debug(socket, "read %zu bytes\n", bytes_received);
    return bytes_received;
}

bool socket_con::poll_readable(uint32_t timeout) {
    pollfd descriptor = {};
    descriptor.fd = fd;
    descriptor.events = POLLIN;
    if (lwip_poll(&descriptor, 1, timeout) == 1) {
        return descriptor.revents & POLLIN;
    } else {
        return false;
    }
}

} // namespace http
