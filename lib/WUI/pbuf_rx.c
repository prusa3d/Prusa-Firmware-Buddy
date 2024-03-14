#include "pbuf_rx.h"

#include "memp.h"

static void pbuf_free_small(struct pbuf *p) {
    memp_free(MEMP_PBUF_POOL_SMALL, p);
}

static struct pbuf *pbuf_alloc_small(u16_t length) {
    struct pbuf_custom *p = (struct pbuf_custom *)memp_malloc(MEMP_PBUF_POOL_SMALL);
    if (p) {
        p->custom_free_function = pbuf_free_small;
        void *payload_mem = ((uint8_t *)p) + LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf_custom));
        u16_t payload_mem_len = LWIP_MEM_ALIGN_SIZE(PBUF_POOL_SMALL_BUFSIZE);
        return pbuf_alloced_custom(PBUF_RAW, length, PBUF_POOL, p, payload_mem, payload_mem_len);
    } else {
        return NULL;
    }
}

struct pbuf *pbuf_alloc_rx(u16_t length) {
    if (length <= PBUF_POOL_SMALL_BUFSIZE) {
        return pbuf_alloc_small(length);
    } else {
        return pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
    }
}
