#pragma once
#include <stdbool.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
enum last_resolve_state_t {
    None = 0,
    Error,
    Resolved,
    Progress
};

/// Syslog UDP transport
///
/// This structure hold state variables for the syslog udp transport. They are not considered a public API and
/// shall not be manipulated directly.
/// Should be zero-initialized before use.
typedef struct {
    /// Whether the transport is ready to send messages.
    bool is_open;

    /// Currently active socket for the transport. -1 if not open.
    int sock;

    /// The remote address to send the syslog messages to.
    struct sockaddr_in addr;

    /// Last reported errno
    int last_errno;

    /// Last logged info
    enum last_resolve_state_t last_resolve_state;
} syslog_transport_t;

/// Prepare a syslog udp transport.
///
/// Returns true on success. In such case, it is the responsibility of the user to call
/// `syslog_transport_close` to free system resources.
bool syslog_transport_open(syslog_transport_t *transport, const char *host, uint16_t port);

/// Get the open state of the transport.
bool syslog_transport_check_is_open(syslog_transport_t *transport);

/// Send a message using given syslog transport. The transport has to be open (see syslog_transport_open).
///
/// Returns true on success. In case it returns false, it is recommmended to close the transport
/// using `syslog_transport_close` and open it again.
bool syslog_transport_send(syslog_transport_t *transport, const char *message, int message_len);

/// Close syslog transport and free system resources.
void syslog_transport_close(syslog_transport_t *transport);

#ifdef __cplusplus
}
#endif //__cplusplus
