#include "ooseq_limit.hpp"

#include <lwip/tcp.h>
#include <lwip/priv/tcp_priv.h>

// Intention behind the limits.
//
// We don't want to get into the situation where all our receive buffers are
// occuppied by out-of-sequence packets. That way an in-sequence packet can
// never come, so it can't unblock the out of sequence ones either - and we
// soft-deadlock the whole receive end (until something actually times out and
// frees stuff - which is easy to prevent by an attacker).
//
// Therefore, we want to somehow limit the amount of out of sequence data we
// allow and start dropping them earlier than we run out of space. But we also
// want to support some out of sequence handling - the Connect connection is
// over the wide Internet (so the out of sequence tends to happen on it more),
// has higher lattency (slower recovery) and sensitive for speed (during GCode
// downloads).
//
// Therefore, we split the connections to two groups:
//
// * Outbount TCP connections (we dial out - the Connect case, single connection).
// * Inbound connections (PrusaLink, multiple connection, possibly created by
//   an attacker).
//
// We give significantly more room to the Connect case.
//
// Note that nothing from here is run or consulted for in-sequence packets at
// all.

namespace {

// Well-known/server ports (0-1023, IANA range); our listeners (PrusaLink
// HTTP on 80, future HTTPS, ...) live here, while outbound connections
// (Connect, downloads) get an ephemeral local_port assigned by lwIP.
constexpr uint16_t WELL_KNOWN_PORT_MAX = 1024;

bool is_inbound(const struct tcp_pcb *pcb) {
    return pcb->local_port < WELL_KNOWN_PORT_MAX;
}

struct OoseqUsage {
    uint32_t all_bytes = 0;
    uint32_t inbound_bytes = 0;
    uint16_t all_segs = 0;
    uint16_t inbound_segs = 0;
};

// Sums ooseq usage of every active pcb except `exclude`. Called from
// tcp_input() with the lwIP core lock held, so tcp_active_pcbs and the
// per-pcb ooseq chains are stable for the duration of the walk.
OoseqUsage other_pcbs_usage(const struct tcp_pcb *exclude) {
    OoseqUsage usage;
    for (const struct tcp_pcb *pcb = tcp_active_pcbs; pcb != nullptr; pcb = pcb->next) {
        if (pcb == exclude) {
            continue;
        }
        const bool inbound = is_inbound(pcb);
        for (const struct tcp_seg *seg = pcb->ooseq; seg != nullptr; seg = seg->next) {
            const uint32_t bytes = seg->p->tot_len;
            usage.all_bytes += bytes;
            usage.all_segs++;
            if (inbound) {
                usage.inbound_bytes += bytes;
                usage.inbound_segs++;
            }
        }
    }
    return usage;
}

} // namespace

extern "C" {

uint32_t buddy_ooseq_bytes_limit(const struct tcp_pcb *pcb) {
    const OoseqUsage usage = other_pcbs_usage(pcb);
    return ooseq_limit::compute_bytes_limit(is_inbound(pcb), usage.all_bytes, usage.inbound_bytes);
}

uint16_t buddy_ooseq_pbufs_limit(const struct tcp_pcb *pcb) {
    const OoseqUsage usage = other_pcbs_usage(pcb);
    return ooseq_limit::compute_segs_limit(is_inbound(pcb), usage.all_segs, usage.inbound_segs);
}

} // extern "C"
