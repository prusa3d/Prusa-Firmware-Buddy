#include "wui_helper_funcs.h"
#include "string.h"
#include "dbg.h"

extern osMessageQId tcp_wui_mpool_id;
extern osSemaphoreId tcp_wui_semaphore_id;

uint32_t send_request_to_wui(wui_cmd_t * req_ptr) {

    osSemaphoreWait(tcp_wui_semaphore_id, osWaitForever);
    if (0 != tcp_wui_queue_id) // queue valid
    {
        uint32_t q_space = osMessageAvailableSpace(tcp_wui_queue_id);
        if (q_space < sizeof(wui_cmd_t)){
            _dbg("message queue to wui full");
            return 1;
        }
        wui_cmd_t * ptr = osPoolAlloc(tcp_wui_mpool_id);
        memcpy(ptr, req_ptr, sizeof(wui_cmd_t));
        osMessagePut(tcp_wui_queue_id, (uint32_t)ptr, osWaitForever); // Send Message
        osDelay(100);
    }
    osSemaphoreRelease(tcp_wui_semaphore_id);
    return 0;
}