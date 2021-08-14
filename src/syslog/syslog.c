#include <errno.h>
#include "syslog.h"
#include "log.h"

LOG_COMPONENT_DEF(Syslog, SEVERITY_INFO);

bool syslog_transport_open(syslog_transport_t *transport, const char *ip_address, int port) {
    transport->sock = -1;
    transport->is_open = false;

    if (strlen(ip_address) == 0)
        return false;
    if (netif_default == NULL)
        return false;

    // open udp socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        log_info(Syslog, "socket: error %i", errno);
        return false;
    }
    transport->sock = sock;

    // prepare remote address
    memset(&transport->addr, 0, sizeof(transport->addr));
    transport->addr.sin_family = AF_INET;
    transport->addr.sin_addr.s_addr = inet_addr(ip_address);
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
        if (errno != EHOSTUNREACH)
            log_info(Syslog, "sendto: error %i", errno);
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
