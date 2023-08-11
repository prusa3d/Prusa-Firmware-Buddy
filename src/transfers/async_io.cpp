#include "async_io.hpp"

#include <cassert>
#include <tasks.hpp>
#include <lwip/tcpip.h>

#define EVER \
    ;        \
    ;

namespace async_io {

namespace {

    // Holds items of type (Request *).
    QueueHandle_t requests = nullptr;

    // Even though we are using the „blocking“ variant (eg. not _try), it
    // is, at least by reading the code, possible this would consume the
    // internal buffers for messages because it allocates that message
    // semi-dynamically from a mem pool :-(.
    //
    // We can't afford to ever lose the callback, but we are allowed to
    // block here and the occurence is probably only theoretical, so wait a
    // bit if it happens (so the tcpip thread chews through few of the
    // messages there and frees something) and retry.
    void tcpip_callback_nofail(tcpip_callback_fn function, void *ctx) {
        while (tcpip_callback(function, ctx) != ERR_OK) {
            osDelay(10);
        }
    }

    void invoke_callback(void *param) {
        static_cast<Request *>(param)->callback();
    }

    void invoke_final_callback(void *param) {
        static_cast<Request *>(param)->final_callback();
    }

} // namespace

void run() {
    Request *received;
    requests = xQueueCreate(REQ_CNT, sizeof received);
    // TODO: null-handling?
    TaskDeps::provide(TaskDeps::Dependency::async_io_ready);

    for (EVER) {
        if (xQueueReceive(requests, &received, portMAX_DELAY) == pdTRUE) {
            while (received->io_task()) {
                // This may block, but that's OK, the enqueue (other direction) is non-blocking.
                tcpip_callback_nofail(invoke_callback, received);
            }
            // One more after the io_task that said stop.
            tcpip_callback_nofail(invoke_final_callback, received);
        }
        // The theory is that portMAX_DELAY means no timeout at all and
        // therefore pdFALSE can't happen, but better safe than sorry
    }
}

void enqueue(Request *request) {
    // Yes, we pass a pointer to pointer and that is intended.
    // Yes, force non-blocking operation.
    auto result = xQueueSend(requests, &request, 0);
    // The caller must ensure not too many requests are enqueued at the same
    // time and blocking doesn't occur.
    assert(result == pdTRUE);
    (void)result; // Prevent unused warning if no assert
}

} // namespace async_io
