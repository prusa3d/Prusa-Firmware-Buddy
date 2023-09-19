#include "syslog_transport.hpp"

#include <lwip/api.h>
#include <dns.h>

void SyslogTransport::reopen(const char *host, uint16_t port) {
    std::unique_lock lock { mutex };

    strlcpy(remote_host, host, sizeof(remote_host));
    remote_port = port;

    close_unlocked();
    open_unlocked();
}

void SyslogTransport::send(const char *message, int message_len) {
    std::unique_lock lock { mutex };

    if (sock < 0) {
        open_unlocked();
    }
    if (sock >= 0) {
        int retval = lwip_sendto(sock, message, message_len, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (retval < 0) {
            close_unlocked();
        }
    }
}

void SyslogTransport::close_unlocked() {
    if (sock >= 0) {
        lwip_close(sock);
        sock = -1;
    }
}

void SyslogTransport::open_unlocked() {
    // bail if disabled
    if (remote_port == 0) {
        return;
    }

    // resolve hostname
    ip_addr_t ip4_addr;
    if (err_t err = netconn_gethostbyname(remote_host, &ip4_addr); err != ERR_OK) {
        return;
    }

    // open udp socket
    sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return;
    }

    // prepare remote address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip4_addr.addr;
    addr.sin_port = htons(remote_port);
}

const char *SyslogTransport::get_remote_host() {
    std::unique_lock lock { mutex };

    return remote_host;
}

uint16_t SyslogTransport::get_remote_port() {
    std::unique_lock lock { mutex };

    return remote_port;
}
