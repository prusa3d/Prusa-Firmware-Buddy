#include "pbuf_rx.h"

#include <ring_allocator.hpp>
#include <freertos/mutex.hpp>
#include <device/board.h>

namespace {

// Standard Ethernet frames are up to 1518 bytes (1500 IP + 14 Eth header + 4 CRC-stripped);
// 1536 covers this with margin.
constexpr size_t mtu_size = 1536;
constexpr size_t alloc_record = 4;
constexpr size_t pbuf_head = LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf_custom));

// Boards with Ethernet (XBUDDY / XLBUDDY, 192 KB RAM) need a larger pool: large UDP datagrams
// fragment into up to 4 frames each, and multiple concurrent broadcast sources can exhaust the
// pool before lwIP finishes reassembly, causing ICMP "Fragment Reassembly Time Exceeded".
// The Buddy board (MINI, 128 KB RAM) uses WiFi only; keep a smaller pool to stay within budget.
#if BOARD_IS_BUDDY()
constexpr size_t mtu_cnt = 10;
constexpr size_t buffer_tail = 500;
#else
constexpr size_t mtu_cnt = 32;
constexpr size_t buffer_tail = 2000;
#endif

// A bit more for "small" packets.
constexpr size_t buffer_size = mtu_cnt * (mtu_size + alloc_record + pbuf_head) + buffer_tail;

alignas(buddy::RingAllocator::alignment) std::array<uint8_t, buffer_size> rx_allocator_buffer;
buddy::RingAllocator rx_allocator(rx_allocator_buffer);

freertos::Mutex allocator_mutex;

void rx_free(struct pbuf *p) {
    std::unique_lock<freertos::Mutex> lock(allocator_mutex);
    rx_allocator.free(p);
}

} // namespace

struct pbuf *pbuf_alloc_rx(u16_t length) {
    std::unique_lock<freertos::Mutex> lock(allocator_mutex);
    size_t req_size = pbuf_head + length;
    struct pbuf_custom *p = static_cast<pbuf_custom *>(rx_allocator.allocate(req_size));
    if (p) {
        p->custom_free_function = rx_free;
        void *payload_mem = ((uint8_t *)p) + pbuf_head;
        return pbuf_alloced_custom(PBUF_RAW, length, PBUF_POOL, p, payload_mem, length);
    } else {
        return nullptr;
    }
}
