#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern uint32_t heap_total_size;
extern uint32_t heap_bytes_remaining;

uint32_t mem_is_heap_allocated(const void *ptr);

#ifdef __cplusplus
}
#endif //__cplusplus
