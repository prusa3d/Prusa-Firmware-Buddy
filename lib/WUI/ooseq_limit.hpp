#pragma once

#include <algorithm>
#include <cstdint>

/// Pure arithmetic behind the direction-aware, globally-bounded TCP
/// out-of-order queue caps (see TCP_OOSEQ_BYTES_LIMIT / TCP_OOSEQ_PBUFS_LIMIT
/// in lwipopts.h and ooseq_limit.cpp). Kept free of lwIP types (inline, no
/// lwIP includes) so it can be exercised by a host unit test.
namespace ooseq_limit {

// Ring bytes budget (protects the shared RX pbuf ring, pbuf_rx.cpp). Worst-case
// pinned ring ~= GLOBAL_BYTES + GLOBAL_SEGS * per-frame overhead; keep it well
// under the ring size in pbuf_rx.cpp.
inline constexpr uint32_t GLOBAL_BYTES = 4096;
inline constexpr uint32_t INBOUND_SHARE_BYTES = 1024;
inline constexpr uint32_t INBOUND_PCB_BYTES = 512;
inline constexpr uint32_t OUTBOUND_PCB_BYTES = 4096;

// tcp_seg count budget (protects the shared MEMP_NUM_TCP_SEG pool).
inline constexpr uint16_t GLOBAL_SEGS = 5;
inline constexpr uint16_t INBOUND_SHARE_SEGS = 2;
inline constexpr uint16_t INBOUND_PCB_SEGS = 1;
inline constexpr uint16_t OUTBOUND_PCB_SEGS = 4;

namespace detail {
    template <typename T>
    inline T room(T budget, T used) {
        return used < budget ? budget - used : 0;
    }
} // namespace detail

/// Max ooseq payload bytes a pcb of the given class may keep, given the
/// ooseq bytes already held by every other pcb.
inline uint32_t compute_bytes_limit(bool is_inbound, uint32_t other_all_bytes, uint32_t other_inbound_bytes) {
    const uint32_t global_room = detail::room(GLOBAL_BYTES, other_all_bytes);
    if (is_inbound) {
        const uint32_t inbound_room = detail::room(INBOUND_SHARE_BYTES, other_inbound_bytes);
        return std::min({ INBOUND_PCB_BYTES, inbound_room, global_room });
    } else {
        return std::min(OUTBOUND_PCB_BYTES, global_room);
    }
}

/// Max ooseq segments a pcb of the given class may keep, given the ooseq
/// segments already held by every other pcb.
inline uint16_t compute_segs_limit(bool is_inbound, uint16_t other_all_segs, uint16_t other_inbound_segs) {
    const uint16_t global_room = detail::room(GLOBAL_SEGS, other_all_segs);
    if (is_inbound) {
        const uint16_t inbound_room = detail::room(INBOUND_SHARE_SEGS, other_inbound_segs);
        return std::min({ INBOUND_PCB_SEGS, inbound_room, global_room });
    } else {
        return std::min(OUTBOUND_PCB_SEGS, global_room);
    }
}

} // namespace ooseq_limit
