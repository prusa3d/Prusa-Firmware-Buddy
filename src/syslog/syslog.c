#include <errno.h>
#include "syslog.h"
#include "log.h"
#include "dns.h"

LOG_COMPONENT_DEF(Syslog, LOG_SEVERITY_INFO);

static void report_error(syslog_transport_t *transport, const char *message_prefix, int error) {
    if (transport->last_errno != error) {
        log_warning(Syslog, "%s: %i", message_prefix, error);
        transport->last_errno = error;
    }
}

static bool syslog_transport_open_ip4(syslog_transport_t *transport, ip_addr_t ip_address, int port) {
    transport->sock = -1;
    transport->is_open = false;

    if (netif_default == NULL)
        return false;

    // open udp socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        report_error(transport, "socket: error", errno);
        return false;
    }
    transport->sock = sock;

    // prepare remote address
    memset(&transport->addr, 0, sizeof(transport->addr));
    transport->addr.sin_family = AF_INET;
    transport->addr.sin_addr.s_addr = ip_address.addr;
    transport->addr.sin_port = htons(port);

    transport->is_open = true;

    return true;
}

bool syslog_transport_check_is_open(syslog_transport_t *transport) {
    return transport->is_open;
}

bool syslog_transport_send(syslog_transport_t *transport, const char *message, int message_len) {
    if (!syslog_transport_check_is_open(transport))
        return false;

    int retval = sendto(transport->sock, message, message_len, 0, (struct sockaddr *)&transport->addr, sizeof(transport->addr));

    if (retval < 0) {
        report_error(transport, "sendto: error", errno);
        return false;
    } else {
        return true;
    }
}

void syslog_transport_close(syslog_transport_t *transport) {
    if (transport->is_open && transport->sock >= 0) {
        close(transport->sock);
        transport->sock = -1;
    }
    transport->is_open = false;
}

bool syslog_transport_open(syslog_transport_t *transport, const char *host, int port) {
    ip_addr_t addr;
    if (strlen(host) == 0)
        return false;
    err_t res = dns_gethostbyname(host, &addr, NULL, NULL);
    if (res == ERR_OK) {
        // ip address already cached or the name was valid ip address
        if (transport->last_resolve_state != Resolved) {
            char ip_addres[16];
            ip4addr_ntoa_r(&addr, ip_addres, 16);
            log_info(Syslog, "Host name %s resolved to this ip: %s", host, ip_addres);
            transport->last_resolve_state = Resolved;
        }
        return syslog_transport_open_ip4(transport, addr, port);
    }
    if (res == ERR_INPROGRESS) {
        // dns lookup queued, wait for it to finish
        log_info(Syslog, "DNS resolving for host %s was enqueued", host);
        transport->last_resolve_state = Progress;
        return false;
    }
    // failed return false
    log_error(Syslog, "DNS resolving failed with value %s", host);
    transport->last_resolve_state = Error;
    return false;
}
