#include "pbuf_rx.h"

#include <ring_allocator.hpp>
#include <freertos/mutex.hpp>

namespace {

// TODO: The size (derive from somewhere)
constexpr size_t mtu_size = 1024;
constexpr size_t alloc_record = 4;
constexpr size_t pbuf_head = LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf_custom));
constexpr size_t mtu_cnt = 10;

// A bit more for "small" packets.
constexpr size_t buffer_size = mtu_cnt * (mtu_size + alloc_record + pbuf_head) + 500;

buddy::RingAllocator rx_allocator(buffer_size);
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
