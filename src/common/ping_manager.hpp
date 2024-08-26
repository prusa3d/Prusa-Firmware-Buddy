#pragma once

#include <memory>

#include <lwip/ip4.h>

struct raw_pcb;

/// A class that performs multiple periodic ping sessions (in parallel).
///
/// The expected usage:
///
/// The caller creates this with expected number of parallel sessions, fills
/// them with either set_ip or set_host. This thing starts pinging it in the
/// tcpip thread. The caller then can request current statistics by the
/// get_stats.
///
/// Deleting the class cancens the pings.
///
/// All the pings are run "in tandem" in interval of 3 seconds. All the
/// parallel pings are the same (same sequence number and id).
class PingManager {
public:
    /// Statistics of one slot.
    struct Stat {
        /// Number of pings sent (excluding eg. out of memory errors during sending).
        size_t cnt = 0;
        /// Number of successful pong responses (duplicates removed).
        size_t success = 0;
        /// Sum of the time of all successes took to get back. That is, average for one is ms_total/successes.
        size_t ms_total = 0;
        /// Number of pings we failed to send (eg. out of memory).
        size_t send_err = 0;
        // Info about the last ping we sent out.
        uint32_t last_sent_time = 0;
        uint16_t last_seq = 0;
        /// The IP to ping. ADDR_ANY means disabled slot (not pinging).
        ///
        /// Could be not set yet with set_ip, or DNS resolution is still running (set_host).
        ip4_addr_t ip = *IP4_ADDR_ANY;
        /// Are we awaiting a response to come?
        ///
        /// (Duplicate protection primarily, but also, the last one can be
        /// excluded from cnt to deduce lost packets).
        bool awaiting = false;

        /// Success rate (in percent)
        uint8_t rate() const;

        /// Average latency for a success
        unsigned latency() const;

        /// We want to keep the stats somewhat actual, not averaged total.
        /// So we do raking after a certain amount of stats
        void maybe_aggregate();

    private:
        friend class PingManager;
        // Used during asynchronous DNS resolution.
        PingManager *owner;

        // TODO: Do we want a jitter too?
    };

    PingManager(size_t nslots);
    /// Asks the tcpip thread to perform cleanup and blocks until it confirms.
    ///
    /// Can also block until previous DNS resolves are finished (since there's
    /// no way to cancel them).
    ~PingManager();
    PingManager(const PingManager &) = delete;
    PingManager(PingManager &&) = delete;
    PingManager &operator=(const PingManager &) = delete;
    PingManager &operator=(PingManager &&) = delete;

    /// Get a copy of the current stats.
    ///
    /// Expects an array of nslots items.
    void get_stats(Stat *stats);
    /// Set a slot to this IP.
    ///
    /// Will reset stats of previous IP if there was any.
    void set_ip(size_t slot, ip4_addr_t ip);
    /// Starts a resolution of the given host and sets the slot to that IP.
    ///
    /// Will _not_ re-resolve the IP later.
    ///
    /// *WARNING*: UB to call this twice on the same slot.
    void set_host(size_t slot, const char *host);

private:
    // We need to clean up in the tcpip thread before we are allowed to finish our delete.
    // This "ows" one token for the pcb to be finished and one for each DNS
    // request in process.
    StaticSemaphore_t delete_sem_buff;
    SemaphoreHandle_t delete_sem;
    /// Protects the statistics.
    ///
    /// The rest is either private to the tcpip thread, read-only.
    freertos::Mutex mutex;
    /// Current statistics / slots.
    ///
    /// nslots long.
    std::unique_ptr<Stat[]> stats;

    /// The "raw socket"
    struct raw_pcb *pcb = nullptr;

    // Random on construction, never changes.
    const uint16_t id;
    // Sequence number of the ping (contains the current round).
    uint16_t seq = 0;
    // While sending, index of the thing we are sending out.
    uint32_t idx = 0;
    const size_t nslots;

    // The below functions are run in the tcpip thread, as callbacks.
    void init();

    // Recv a packet from the raw socket
    // (possibly responses for our pings)
    uint8_t recv(struct pbuf *p, const ip_addr_t *addr);

    // Sends out the next round of pings
    void round();
    static void round_wrap(void *arg);

    void send_ping(size_t idx);

    // Called from destructor, to perform cleanup inside the tcpip thread
    // Destructor is blocked until this completes.
    void finish();
    void set_host_inner(size_t slot, const char *host);
    void dns_found(const ip_addr_t *ip, Stat *st);
};
