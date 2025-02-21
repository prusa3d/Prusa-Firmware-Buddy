#include "ping_manager.hpp"
#include "timing.h"

#include <freertos/binary_semaphore.hpp>
#include <common/tcpip_callback_nofail.hpp>
#include <common/random.h>
#include <common/pbuf_deleter.hpp>
#include <logging/log.hpp>

#include <lwip/inet.h>
#include <lwip/inet_chksum.h>
#include <lwip/ip.h>
#include <lwip/icmp.h>
#include <lwip/raw.h>
#include <lwip/dns.h>

#include <mutex>
#include <memory>

using std::lock_guard;
using std::unique_ptr;

LOG_COMPONENT_DEF(Ping, logging::Severity::info);

namespace {

// Ping every 3s
constexpr const uint32_t ping_round = 3000;
constexpr const uint32_t ping_spread = 100;

} // namespace

uint8_t PingManager::Stat::rate() const {
    const size_t real_cnt = cnt - awaiting;
    if (real_cnt == 0) {
        return 0;
    } else {
        return 100 * success / real_cnt;
    }
}

unsigned PingManager::Stat::latency() const {
    if (success == 0) {
        return 0;
    } else {
        return ms_total / success;
    }
}

PingManager::PingManager(size_t nslots)
    : delete_sem(xSemaphoreCreateCountingStatic(nslots + 1, 0, &delete_sem_buff))
    , id(rand_u())
    , nslots(nslots) {
    stats.reset(new Stat[nslots]);
    tcpip_callback_nofail([](void *arg) {
        reinterpret_cast<PingManager *>(arg)->init();
    },
        this);
}

PingManager::~PingManager() {
    tcpip_callback_nofail([](void *arg) {
        reinterpret_cast<PingManager *>(arg)->finish();
    },
        this);
    // One token is held by the PCB (removed in finish, held for the whole time)
    // One is held by each asynchronous DNS resolution (not held most of the
    //    time); each slot can have one DNS resolution.
    for (size_t i = 0; i < nslots + 1; i++) {
        xSemaphoreTake(delete_sem, portMAX_DELAY);
    }
}

void PingManager::get_stats(Stat *stats) {
    lock_guard lock(mutex);
    memcpy(stats, this->stats.get(), nslots * sizeof *stats);
}

void PingManager::set_ip(size_t slot, ip4_addr_t ip) {
    assert(slot < nslots);

    auto &stat = stats[slot];
    if (stat.ip.addr == ip.addr) {
        return;
    }

    stat = Stat();
    stat.ip = ip;
}

void PingManager::dns_found(const ip_addr_t *ip, Stat *st) {
    // OK to do early, we are in the tcpip thread and the last one is given in
    // the finish which also runs in this thread.
    xSemaphoreGive(delete_sem);

    if (ip == nullptr || !IP_IS_V4(addr)) {
        return;
    }

    lock_guard lock(mutex);
    ip4_addr_copy(st->ip, *ip_2_ip4(ip));
}

void PingManager::set_host_inner(size_t slot, const char *host) {
    // Reselve a slot for async DNS.
    xSemaphoreTake(delete_sem, portMAX_DELAY);

    lock_guard lock(mutex);

    stats[slot] = Stat();
    stats[slot].owner = this;
    err_t dns_result = dns_gethostbyname(
        host, &stats[slot].ip, [](const char *, const ip_addr_t *ip, void *arg) {
            Stat *st = reinterpret_cast<Stat *>(arg);
            // Accessing before locking... This is fine as long as someone doesn't call
            // the method twice on the same slot, which we forbid.
            st->owner->dns_found(ip, st);
        },
        &stats[slot]);

    switch (dns_result) {
    case ERR_INPROGRESS:
        // This will solve everything in the callback, including returning the semaphore.
        break;
    default:
        // Including the OK case, in which the IP is directly set.
        // There's also possible other-error case which shouldn't happen, but
        // it would leave the slot in "inactive" mode.
        xSemaphoreGive(delete_sem);
        break;
    }
}

void PingManager::set_host(size_t slot, const char *host) {
    assert(slot < nslots);
    freertos::BinarySemaphore semaphore;
    struct SetHostData {
        PingManager *manager;
        const char *host;
        size_t slot;
        freertos::BinarySemaphore *semaphore;
    } data = {
        .manager = this,
        .host = host,
        .slot = slot,
        .semaphore = &semaphore,
    };

    // Executed the body of the resolution in the tcpip thread
    tcpip_callback_nofail([](void *arg) {
        SetHostData *data = reinterpret_cast<SetHostData *>(arg);
        data->manager->set_host_inner(data->slot, data->host);
        data->semaphore->release();
    },
        &data);
    // And wait for it to finish (so we can release the data and possibly the
    // host on caller side).
    semaphore.acquire();
}

void PingManager::init() {
    // Not locked on purpose, we do _not_ touch any of the "shared" structures
    // (which are mostly the ping statistics). The round locks internally.
    pcb = raw_new(IP_PROTO_ICMP);

    if (pcb == nullptr) {
        // TODO Signal error
        return;
    }

    raw_bind(pcb, IP_ADDR_ANY);
    raw_recv(
        pcb, [](void *arg, struct raw_pcb *, struct pbuf *p, const ip_addr_t *addr) -> uint8_t {
            return reinterpret_cast<PingManager *>(arg)->recv(p, addr);
        },
        this);

    // Once we are initialized, we can start processing DNS requests; also,
    // this disallows finish before init (which is probably not possible
    // anyway).
    for (size_t i = 0; i < nslots; i++) {
        xSemaphoreGive(delete_sem);
    }

    // Send the first pings out and schedule following rounds
    round();
}

uint8_t PingManager::recv(struct pbuf *p, const ip_addr_t *addr) {
    // 1. Parse it and see if it's ICMP reply with correct sequence number (we can access that one, only the tcpip thread accesses that)
    // 2. Lock the mutex, assign to specific slot, update that one (if one matches)
    //
    // We do the locking after preliminary checks to avoid the locking if not necessary.
    if (!IP_IS_V4(addr)) {
        return 0;
    }
    // We get the packet including the IP headers. We need to skip them and
    // they are variable length, based on the header length field.
    struct ip_hdr ip;
    uint16_t copied = pbuf_copy_partial(p, &ip, sizeof ip, 0);
    if (copied < sizeof ip) {
        return 0;
    }
    uint8_t skip = 4 * IPH_HL(&ip);

    struct icmp_echo_hdr echo;
    copied = pbuf_copy_partial(p, &echo, sizeof echo, skip);
    if (copied < sizeof echo) {
        // Packet too short to contain echo response
        return 0;
    }

    size_t slot = echo.id - id;
    if (echo.type != ICMP_ER || slot >= nslots) {
        return 0;
    }

    // OK, it looks like it is an echo response for one of our requests.

    lock_guard lock(mutex);
    auto &stat = stats[slot];

    if (PP_HTONS(stat.last_seq) != echo.seqno) {
        return 0;
    }

    if (ip_2_ip4(addr)->addr == stat.ip.addr && stat.awaiting) {
        uint32_t duration = ticks_ms() - stat.last_sent_time;
        stat.ms_total += duration;
        stat.success++;
        stat.awaiting = false;
        stat.maybe_aggregate();
        pbuf_free(p);
        return 1;
    }

    return 0;
}

void PingManager::round_wrap(void *arg) {
    reinterpret_cast<PingManager *>(arg)->round();
}

// Note: Already locked by the caller
void PingManager::send_ping(size_t idx) {
    Stat &stat = stats[idx];

    const uint16_t cur_seq = PP_HTONS(seq);
    struct icmp_echo_hdr echo = {};
    echo.type = ICMP_ECHO;
    echo.code = 0;
    echo.seqno = cur_seq;

    ip_addr_t addr;
    IP_SET_TYPE_VAL(addr, IPADDR_TYPE_V4);

    if (stat.ip.addr == IP4_ADDR_ANY->addr) {
        return;
    }

    unique_ptr<pbuf, PbufDeleter> pkt(pbuf_alloc(PBUF_IP, sizeof echo, PBUF_RAM));
    if (!pkt) {
        log_warning(Ping, "Failed to allocate packet");
        stat.send_err++;
        stat.maybe_aggregate();
        return;
    }
    echo.id = id + idx;
    // The checksum is computed from the checksum field too, we need to
    // set it to 0 for it to come out correctly.
    echo.chksum = 0;
    echo.chksum = inet_chksum(&echo, sizeof echo);
    memcpy(pkt->payload, &echo, sizeof echo);

    ip4_addr_copy(*ip_2_ip4(&addr), stat.ip);
    auto err = raw_sendto(pcb, pkt.get(), &addr);
    if (err == ERR_OK) {
        stat.last_sent_time = ticks_ms();
        stat.last_seq = seq;
        stat.cnt++;
        stat.awaiting = true;
    } else {
        log_warning(Ping, "Failed to send ping");
        stat.send_err++;
    }
}

void PingManager::round() {
    lock_guard lock(mutex);

    if (idx >= nslots) {
        // Schedule the next round of pings
        uint32_t wait_time = ping_round - nslots * ping_spread;
        if (nslots * ping_spread >= ping_round) {
            wait_time = 0;
        }
        sys_timeout(wait_time, PingManager::round_wrap, this);
        seq++;
        idx = 0;
    } else {
        // Next ping in the round, just spread them a bit so they don't leave all at once.
        sys_timeout(ping_spread, PingManager::round_wrap, this);
        send_ping(idx);
        idx++;
    }
}

void PingManager::finish() {
    if (pcb != nullptr) {
        raw_remove(pcb);
        pcb = nullptr;
    }

    sys_untimeout(PingManager::round_wrap, this);

    xSemaphoreGive(delete_sem);
}

void PingManager::Stat::maybe_aggregate() {
    if (cnt < 16) {
        return;
    }

    cnt /= 2;
    success /= 2;
    ms_total /= 2;
    send_err /= 2;
}
