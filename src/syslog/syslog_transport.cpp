#include "syslog_transport.hpp"
#include <tcpip.h>

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
    dns_resolved = DnsState::None;
    if (sock >= 0) {
        lwip_close(sock);
        sock = -1;
    }
}

namespace {
static void dns_found_wrap(const char *, const ip_addr_t *ip, void *arg) {
    static_cast<SyslogTransport *>(arg)->dns_done_callback(ip);
}

static void find_dns(void *ctx) {
    static_cast<SyslogTransport *>(ctx)->dns_resolve();
}
} // namespace

void SyslogTransport::open_unlocked() {
    // bail if disabled
    if (remote_port == 0) {
        return;
    }

    if (dns_resolved == DnsState::None) {
        auto res = tcpip_try_callback(find_dns, this);
        if (res != ERR_OK) {
            // We will try again on next send
            return;
        }
    } else if (dns_resolved == DnsState::Progress) {
        return;
    } else /* if (dns_resolved == DnsState::Resolved)*/ {
        // open udp socket
        sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    }
}

const char *SyslogTransport::get_remote_host() {
    std::unique_lock lock { mutex };

    return remote_host;
}

uint16_t SyslogTransport::get_remote_port() {
    std::unique_lock lock { mutex };

    return remote_port;
}

void SyslogTransport::dns_done_callback(const ip_addr_t *ip) {
    if (ip == nullptr) {
        dns_resolved = DnsState::None;
    }
    // prepare remote address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip->addr;
    addr.sin_port = htons(remote_port);
    dns_resolved = DnsState::Resolved;
}

void SyslogTransport::dns_resolve() {
    // resolve hostname
    ip_addr_t ip4_addr;
    err_t dns_result = dns_gethostbyname(remote_host, &ip4_addr, dns_found_wrap, this);
    // TODO result
    switch (dns_result) {
    case ERR_OK:
        dns_done_callback(&ip4_addr);
        break;
    case ERR_INPROGRESS:
        dns_resolved = DnsState::Progress;
        // Callback will be called.
        break;
    default:
        dns_resolved = DnsState::None;
        // Should not happen
    }
}
