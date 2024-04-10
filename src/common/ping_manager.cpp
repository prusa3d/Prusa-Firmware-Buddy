#include "ping_manager.hpp"
#include "timing.h"

#include <common/tcpip_callback_nofail.hpp>
#include <common/random.h>
#include <common/pbuf_deleter.hpp>

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

namespace {

// Ping every 3s
constexpr const uint32_t ping_round = 3000;

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
    StaticSemaphore_t sem_buff;
    SemaphoreHandle_t sem = xSemaphoreCreateBinaryStatic(&sem_buff);
    struct SetHostData {
        PingManager *manager;
        const char *host;
        size_t slot;
        SemaphoreHandle_t semaphore;
    } data = {
        .manager = this,
        .host = host,
        .slot = slot,
        .semaphore = sem,
    };

    // Executed the body of the resolution in the tcpip thread
    tcpip_callback_nofail([](void *arg) {
        SetHostData *data = reinterpret_cast<SetHostData *>(arg);
        data->manager->set_host_inner(data->slot, data->host);
        xSemaphoreGive(data->semaphore);
    },
        &data);
    // And wait for it to finish (so we can release the data and possibly the
    // host on caller side).
    xSemaphoreTake(sem, portMAX_DELAY);
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
    if (echo.type != ICMP_ER || slot >= nslots || echo.seqno != PP_HTONS(seq.load())) {
        return 0;
    }

    // OK, it looks like it is an echo response for one of our requests from
    // the last round. Find the right slot for it.

    lock_guard lock(mutex);
    auto &stat = stats[slot];

    if (ip_2_ip4(addr)->addr == stat.ip.addr && stat.awaiting) {
        uint32_t duration = ticks_ms() - last_round;
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

void PingManager::round() {
    // Reschedule next round
    sys_timeout(ping_round, PingManager::round_wrap, this);

    // Increment the sequence number for the round
    //
    // For each active slot, send the ping out.

    seq.fetch_add(1);
    const uint16_t cur_seq = PP_HTONS(seq.load());
    struct icmp_echo_hdr echo = {};
    echo.type = ICMP_ECHO;
    echo.code = 0;
    echo.seqno = cur_seq;

    ip_addr_t addr;
    IP_SET_TYPE_VAL(addr, IPADDR_TYPE_V4);

    lock_guard lock(mutex);

    last_round = ticks_ms();

    for (size_t i = 0; i < nslots; i++) {
        Stat &stat = stats[i];
        if (stat.ip.addr == IP4_ADDR_ANY->addr) {
            continue;
        }
        // TODO: Do we want to spread out the sending, so we don't run out of small
        // pcbs in one ping "burst"?
        //
        // Also, why can't we reuse the pbuf across multiple pings and have
        // to free it and create a new one? :-O (it creates malformed
        // packets otherwise).
        unique_ptr<pbuf, PbufDeleter> pkt(pbuf_alloc(PBUF_IP, sizeof echo, PBUF_RAM));
        if (!pkt) {
            stat.send_err++;
            stat.maybe_aggregate();
            continue;
        }
        echo.id = id + i;
        // The checksum is computed from the checksum field too, we need to
        // set it to 0 for it to come out correctly.
        echo.chksum = 0;
        echo.chksum = inet_chksum(&echo, sizeof echo);
        memcpy(pkt->payload, &echo, sizeof echo);

        ip4_addr_copy(*ip_2_ip4(&addr), stat.ip);
        auto err = raw_sendto(pcb, pkt.get(), &addr);
        if (err == ERR_OK) {
            stat.cnt++;
            stat.awaiting = true;
        } else {
            stat.send_err++;
        }
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
