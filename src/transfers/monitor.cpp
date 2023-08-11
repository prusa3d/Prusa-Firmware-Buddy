#include "monitor.hpp"
#include "../../lib/WUI/random.h"

#include <timing.h>

#include <algorithm>
#include <cassert>
#include <cstring>

using std::min;
using std::nullopt;
using std::optional;
using std::unique_lock;

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
}

namespace transfers {

Monitor::Slot::Slot(Monitor &owner)
    : owner(owner)
    , live(true) {}

Monitor::Slot::Slot(Slot &&other)
    : owner(other.owner)
    , live(true) { other.live = false; }

Monitor::Slot::~Slot() {
    if (live) {
        Lock lock(owner.main_mutex);
        bool destruction_handled;
        {
            Lock lock2(owner.history_mutex);
            destruction_handled = (owner.current_id == owner.history_latest);
        }

        if (!destruction_handled) {
            done(Outcome::Error);
        }

        owner.transfer_active = false;
    }
}

Monitor::Slot &Monitor::Slot::operator=([[maybe_unused]] Slot &&other) {
    // This is just to satisfy optional<Slot>::operator=, but doesn't do anything really.
    //
    // It's not needed in reality, as we aren't supposed to have two slots at
    // the same time anyway and it would be a bit involved to write - first
    // dropping the old one properly if live, then gutting the new one, the
    // owner would have to be pointer instead of referrence...
    assert(0);
    return *this;
}

void Monitor::Slot::progress(size_t additional_bytes) {
    Lock lock(owner.main_mutex);

    owner.transferred += additional_bytes;
}

void Monitor::Slot::reset_progress() {
    Lock lock(owner.main_mutex);
    owner.transferred = 0;
}

void Monitor::Slot::done(Outcome outcome) {
    Lock lock(owner.history_mutex);

    // Move the old history one item to the past, drop anything that doesn't fit.
    size_t preserved = min(owner.history_len, owner.history.size() - 1);
    memmove(owner.history.begin() + 1, owner.history.begin(), preserved * sizeof *owner.history.begin());
    // Include the new item.
    owner.history[0] = outcome;
    owner.history_len = preserved + 1;
    // We are reading an atomic that changes only when the main_mutex is
    // locked. Note that the change can't happen here even though we _don't_
    // lock the mutex, because the only way it can change is to allocate a new
    // slot ‒ an we can't allocate a slot as long as this slot is alive. So
    // this must contain the correct ID.
    owner.history_latest = owner.current_id;
}

const char *Monitor::Slot::filepath() {
    return owner.destination_path;
}

optional<Monitor::Status> Monitor::status(bool allow_stale) const {
    Lock lock(main_mutex);

    if (!used || (!allow_stale && !transfer_active)) {
        return nullopt;
    }

    Status result { std::move(lock) };
    result.type = type;
    result.id = current_id;
    result.start = start;
    result.expected = expected;
    result.print_after_upload = print_after_upload;
    result.transferred = transferred;
    result.destination = strlen(destination_path) > 0 ? destination_path : nullptr;

    return result;
}

optional<Monitor::Outcome> Monitor::outcome(TransferId id) const {
    Lock lock(history_mutex);

    // How much to the past into the history do we go?
    uint32_t offset = history_latest - id;
    if (offset > history_len) {
        // The history is continuous segment back. We may be asked for outdated
        // ID (in which case it'll be slightly larger than the length of the
        // history). We may also be asked for completely bogus ID, in wich case
        // it may even underflow ‒ and will likely be out of bounds.
        return nullopt;
    }

    return history[offset];
}

optional<TransferId> Monitor::id() const {
    // This loop may look a bit weird. That's because we are dealing with
    // atomics, multiple threads, etc. The check that the ID stayed the same is
    // to ensure we don't report nonsense around the time one transfer ends and
    // another starts.
    for (;;) {
        TransferId guess = current_id;
        if (transfer_active) {
            if (guess == current_id) {
                return guess;
            }
        } else {
            return nullopt;
        }
    }
}

optional<Monitor::Slot> Monitor::allocate(Type type, const char *dest, size_t expected_size, bool print_after_upload) {
    Lock lock(main_mutex);

    if (transfer_active) {
        // There's another tranfer ongoing, can't start another one.
        return nullopt;
    }

    if (current_id == 0) {
        // Special-case: lazy initialize by a random number. Initialization in
        // the construction was too early and the HW generator wasn't ready.
        // Using the usual `random` call also produced predictable, always the
        // same results.
        //
        // Yes, if we overflow to 0, we re-initialize it to some random number,
        // but, not a big deal.
        uint32_t init = 0;
        random32bit(&init);
        current_id = init;
    }

    // Order matters, these are atomics, and observable from another thread.
    // First change the ID before „activating“ the transfer.
    current_id++;
    transfer_active = true;
    used = true;
    stopped = false;

    // Store the details.
    this->type = type;
    expected = expected_size;
    transferred = 0;
    this->print_after_upload = print_after_upload;
    start = ticks_s();
    if (dest != nullptr) {
        strlcpy(destination_path, dest, sizeof(destination_path));
    } else {
        destination_path[0] = '\0';
    }

    return Slot(*this);
}

bool Monitor::signal_stop() {
    Lock lock(main_mutex);
    if (transfer_active) {
        stopped = true;
        return true;
    }

    return false;
}

bool Monitor::Slot::is_stopped() {
    Lock lock(owner.main_mutex);
    return owner.stopped;
}

Monitor Monitor::instance;

const char *to_str(Monitor::Type type) {
    switch (type) {
    case Monitor::Type::Connect:
        return "FROM_CONNECT";
    case Monitor::Type::Link:
        return "FROM_CLIENT";
    default:
        assert(0);
        return "NO_TRANSFER";
    }
}

double Monitor::Status::progress_estimate() const {
    if (expected > 0) {
        return static_cast<double>(transferred) / static_cast<double>(expected);
    } else {
        return 0;
    }
}

uint32_t Monitor::Status::time_remaining_estimate() const {
    uint32_t elapsed = ticks_s() - start;
    double progress = progress_estimate();
    if (progress > 0.0) {
        double total_expected = elapsed / progress;
        return total_expected - elapsed;
    } else {
        // No estimate yet, just give them a 0
        return 0;
    }
}

uint32_t Monitor::Status::time_transferring() const {
    return ticks_s() - start;
}

} // namespace transfers
