#pragma once

#include <common/freertos_mutex.hpp>
#include <config_store/constants.hpp>
#include <sys/socket.h>

#include <atomic>

struct SyslogTransport {
private:
    int sock = -1;
    struct sockaddr_in addr;
    freertos::Mutex mutex;
    char remote_host[config_store_ns::connect_host_size + 1] = "";
    uint16_t remote_port = 0;

public:
    /**
     * Reconfigure the instance and reopen connection.
     *
     * Synchronized with other public methods via mutex.
     * May block and may fail to reopen connection.
     */
    void reopen(const char *host, uint16_t port);

    /**
     * Send message.
     *
     * Synchronized with other public methods via mutex.
     * May block and may fail to send message.
     */
    void send(const char *message, int message_len);

    /**
     * Return remote_host configured for this instance.
     *
     * Synchronized with other public methods via mutex.
     */
    const char *get_remote_host();

    /**
     * Return remote_port configured for this instance.
     *
     * Synchronized with other public methods via mutex.
     */
    uint16_t get_remote_port();

    void dns_done_callback(const ip_addr_t *ip);
    void dns_resolve();

private:
    void close_unlocked();
    void open_unlocked();

    enum class DnsState {
        None,
        Resolved,
        Progress,
    };

    std::atomic<DnsState> dns_resolved = DnsState::None;
};
