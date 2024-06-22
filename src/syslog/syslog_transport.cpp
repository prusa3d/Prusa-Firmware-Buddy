#include "syslog_transport.hpp"
#include <tcpip.h>

#include <lwip/udp.h>
#include <dns.h>
#include <common/tcpip_callback_nofail.hpp>
#include <common/pbuf_deleter.hpp>
#include <freertos/binary_semaphore.hpp>

using freertos::BinarySemaphore;

namespace {
struct Msg {
    SyslogTransport *me;
    pbuf *message;
};

// A shared pool of messages to be sent.
//
// This is shared between all the SyslogTransport instances,
// because we also share the space in PBUF pools to store the
// messages, share the tcpip message queue we send it through,
// share the tcpip thread itself, etc - so having a separate pool
// for the messages kind of doesn't make sense.
LWIP_MEMPOOL_DECLARE(SYSLOG_MSGS, 4, sizeof(Msg), "Syslog messages waiting for being sent");

std::atomic<bool> pool_initialized = false;

void release(Msg *msg) {
    if (msg) {
        if (msg->message != nullptr) {
            pbuf_free(msg->message);
            msg->message = nullptr;
        }

        LWIP_MEMPOOL_FREE(SYSLOG_MSGS, msg);
    }
}
} // namespace

SyslogTransport::SyslogTransport() {
    // We need to make sure the shared pool is initialized before
    // first use.
    //
    // This is technically a bit iffy/racy - but we assume at least
    // one instance is created before any threads are started (both our
    // instances are now globals and should be initialized before main starts),
    // therefore the race doesn't happen.
    if (!pool_initialized.exchange(true)) {
        LWIP_MEMPOOL_INIT(SYSLOG_MSGS);
    }
}

void SyslogTransport::UdpPcbDeleter::operator()(udp_pcb *p) {
    udp_remove(p);
}

std::unique_ptr<udp_pcb, SyslogTransport::UdpPcbDeleter> SyslogTransport::socket;

void SyslogTransport::reopen(const char *host, uint16_t port) {
    struct Msg {
        SyslogTransport *me;
        const char *host;
        uint16_t port;
        BinarySemaphore semaphore;
    };
    Msg msg = {
        this,
        host,
        port,
        BinarySemaphore(),
    };

    tcpip_callback_nofail([](void *arg) {
        Msg *msg = static_cast<Msg *>(arg);

        strlcpy(msg->me->remote_host, msg->host, sizeof(msg->me->remote_host));
        msg->me->remote_port = msg->port;

        msg->me->close();

        msg->semaphore.release();
    },
        &msg);

    msg.semaphore.acquire();
    initialized = true;
}

void SyslogTransport::send(const char *message, int message_len) {
    if (!initialized) {
        // It was observed the send is sometimes called early - before the
        // tcpip thread is running. At that point, we haven't called reopen
        // yet, so this is just to prevent the BSOD by sending into
        // non-existent tcpip thread.
        return;
    }

    // mempools return void *, which auto-converts in C, but not in
    // C++. The pools promise the same alignment as malloc.
    Msg *msg = static_cast<Msg *>(LWIP_MEMPOOL_ALLOC(SYSLOG_MSGS));

    if (!msg) {
        // Out of memory for the messages (there are too many in
        // flight already)
        return;
    }

    msg->me = this;
    msg->message = pbuf_alloc(PBUF_TRANSPORT, message_len, PBUF_RAM);
    if (msg->message == nullptr) {
        return release(msg);
    }

    memcpy(msg->message->payload, message, message_len);

    err_t post = tcpip_try_callback([](void *arg) {
        Msg *msg_arg = static_cast<Msg *>(arg);

        msg_arg->me->do_send(msg_arg->message);
        release(msg_arg);
    },
        msg);

    if (post != ERR_OK) {
        // We failed to send the message. Therefore it is up to us to return the buffer.
        release(msg);
    }
}

void SyslogTransport::do_send(pbuf *message) {
    if (remote_port == 0) {
        // Transport disabled
        return;
    }

    if (dns_resolved == DnsState::None) {
        // resolve hostname
        ip_addr_t ip4_addr;
        err_t dns_result = dns_gethostbyname(
            remote_host, &ip4_addr, [](const char *, const ip_addr_t *addr, void *arg) {
                static_cast<SyslogTransport *>(arg)->dns_done_callback(addr);
            },
            this);
        switch (dns_result) {
        case ERR_OK:
            dns_done_callback(&ip4_addr);
            break;
        case ERR_INPROGRESS:
            dns_resolved = DnsState::Progress;
            // Callback will be called.
            break;
        default:
            // Should not happen
            dns_resolved = DnsState::None;
            break;
        }
    }

    if (dns_resolved != DnsState::Resolved) {
        // Still waiting for the IP address. Drop the message, try next time.
        return;
    }

    if (!socket) {
        socket.reset(udp_new());
        if (!socket) {
            // Not enough memory for the UDP socket, try next time.
            return;
        }
        if (udp_bind(socket.get(), IP_ADDR_ANY, 0) != ERR_OK) {
            socket.reset();
            return;
        }
    }

    switch (udp_sendto(socket.get(), message, &addr, remote_port)) {
    case ERR_OK:
    case ERR_MEM:
    case ERR_RTE:
        // These don't mean the PCB is broken, so keep it around.
        break;
    default:
        // Some other (unknown) error, better kill the PCB and start fresh next time.
        close();
        socket.reset();
        break;
    }
}

void SyslogTransport::close() {
    dns_resolved = DnsState::None;
}

void SyslogTransport::dns_done_callback(const ip_addr_t *ip) {
    if (ip == nullptr) {
        dns_resolved = DnsState::None;
    } else {
        addr = *ip;
        dns_resolved = DnsState::Resolved;
    }
}
