#pragma once

#include <cstdio>

// Utility to run blocking tasks asynchronously from the TCPIP thread.
//
// We need to submit writes (especially during a gcode transfer) to the USB,
// which is blocking and potentially long-running operation (seconds). This
// doesn't play well with how the tcpip thread works, if it gets blocked, the
// TCP performance suffers, timeouts do weird things, etc.
//
// For that reason, we send the blocking operations to a separate thread and
// that one lives here.
//
// Few details of note:
//
// * The Request is an abstract base class for the caller to implement the
//   actual requests. It is submitted into a queue.
// * The requests are not allocated nor deleted by this utility. The idea is
//   that they are long-living and reused.
// * The caller is responsible for making sure the queue doesn't overflow. The
//   functions here never block, which prevents deadlocks.
//
// # Expected usage
//
// The caller shall have a pool of requests of at most REQ_CNT items (it can be
// multiple pools of different types of requests). These can be submitted in there.
//
// Once the callback is called, the request can be safely returned to the pool.
//
// A request can be created (and enqueued) if there isn't one in the pool.
//
// # Error handling and cancelation
//
// The utility hear has no notion of fallibility of the operations. The
// requests need to handle this internally (eg store it inside it).
//
// If requests need to be canceled, they also need to do so internally by "soft
// means" (eg. flag to skip everything).
//
// # Thread safety of requests
//
// The request conceptually passes from the caller thread to the IO thread and
// back to tcpip thread at given times and the methods are not called in
// between. That is, requests need to be able to move from one thread to
// another, need to be reentrant, but don't need to be strictly thread safe
// from the point of this utility.
//
// Nevertheless, if they are accessed by the caller while inside this queue, or
// if multiple requests share access to the same data, _that_ needs
// synchronization.

namespace async_io {

// 4 write requests + 1 close request.
//
// Need to be large enough to fit all needed requests, so the sender has no
// chance of blocking.
constexpr size_t REQ_CNT = 8 + 1;

__attribute__((noreturn)) void run();

class Request {
public:
    // Called inside the IO thread.
    //
    // This is multi-shot. It's called again as long as it returns true. The
    // callback is sent to the tcpip thread after each invocation.
    virtual bool io_task() = 0;
    // Called afterwards in the tcp_ip thread.
    //
    // Can be used for returning the ownership back to the caller / returning
    // to the pool.
    virtual void callback() = 0;
    virtual void final_callback() = 0;
    // We don't really expect the requests to get destroyed ever, but mostly
    // for the form of abstract base class...
    virtual ~Request() = default;
};

// Not blocking.
//
// Caller must ensure the queue doesn't overflow (eg. by the size of the pools).
void enqueue(Request *request);

} // namespace async_io
