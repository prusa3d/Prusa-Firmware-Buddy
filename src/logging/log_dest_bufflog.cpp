#include <logging/log_dest_bufflog.hpp>

#include <algorithm>
#include <cstring>
#include <freertos/mutex.hpp>
#include <logging/log_dest_shared.hpp>

#define BUFFLOG_BUFFER_SIZE 256

// Can't use std::unique_lock from <mutex> because it pulls in a bunch of other
// std::crap which fails debug build.
template <class Mutex>
class UniqueLock {
private:
    Mutex &mutex;

public:
    explicit UniqueLock(Mutex &mutex)
        : mutex { mutex } { mutex.lock(); }
    ~UniqueLock() { mutex.unlock(); }
};

namespace logging {

static freertos::Mutex mutex;

typedef struct {
    char *data;
    size_t read;
    size_t write;
} buffer_output_state_t;

static void buffer_output(char character, void *arg) {
    buffer_output_state_t *state = (buffer_output_state_t *)arg;
    const size_t next = (state->write + 1) % BUFFLOG_BUFFER_SIZE;
    if (next != state->read) {
        state->write = next;
        state->data[state->write] = character;
    }
}

static char buffer[BUFFLOG_BUFFER_SIZE];
static buffer_output_state_t buffer_state = {
    .data = buffer,
    .read = 0,
    .write = 0,
};

void bufflog_log_event(FormattedEvent *event) {
    UniqueLock lock { mutex };

    log_format_simple(event, buffer_output, &buffer_state);
    buffer_output(BUFFLOG_TERMINATION_CHAR, &buffer_state);
}

size_t bufflog_pickup(char *dest, size_t buffer_size) {
    UniqueLock lock { mutex };

    const size_t available = (buffer_state.write - buffer_state.read) % BUFFLOG_BUFFER_SIZE;
    const size_t to_copy = std::min(buffer_size, available);

    // Copy data to buffer - read pos to end
    const size_t first_part_size = std::min(to_copy, BUFFLOG_BUFFER_SIZE - buffer_state.read);
    memcpy(dest, buffer + buffer_state.read, first_part_size);
    // Copy data to buffer - begin to write pos
    memcpy(dest + first_part_size, buffer, to_copy - first_part_size);

    buffer_state.read = (buffer_state.read + to_copy) % BUFFLOG_BUFFER_SIZE;

    return to_copy;
}

} // namespace logging
