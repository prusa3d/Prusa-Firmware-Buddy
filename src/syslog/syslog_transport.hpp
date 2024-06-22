#pragma once

#include <config_store/constants.hpp>
#include <sys/socket.h>
#include <memory>

#include <atomic>

struct udp_pcb;

struct SyslogTransport {
private:
    struct UdpPcbDeleter {
        void operator()(udp_pcb *);
    };
    // The socket is used only for sending UDP packets out, not receiving
    // anything. Therefore, we can dare to share a single socket between
    // multiple instances.
    static std::unique_ptr<udp_pcb, UdpPcbDeleter> socket;

    enum class DnsState {
        None,
        Resolved,
        Progress,
    };

    DnsState dns_resolved = DnsState::None;
    ip_addr_t addr;
    char remote_host[config_store_ns::connect_host_size + 1] = "";
    uint16_t remote_port = 0;

    std::atomic<bool> initialized = false;

public:
    SyslogTransport();
    /**
     * Reconfigure the instance and reopen connection.
     *
     * May block and may fail to reopen connection.
     */
    void reopen(const char *host, uint16_t port);

    /**
     * Send message.
     *
     * Doesn't block, but may lose messages (both on the network level
     * and when there are not enough internal buffers).
     */
    void send(const char *message, int message_len);

private:
    void close();

    void do_send(pbuf *message);
    void dns_done_callback(const ip_addr_t *addr);
};
