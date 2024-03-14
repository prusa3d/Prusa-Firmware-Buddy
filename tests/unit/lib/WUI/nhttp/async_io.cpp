// Async-io stubs. They are specifically not async (which shouldn't matter in the tests).

#include <transfers/async_io.hpp>

namespace async_io {

void enqueue(Request *request) {
    request->io_task();
    request->callback();
}

} // namespace async_io
