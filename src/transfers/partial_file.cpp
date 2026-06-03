#include "partial_file.hpp"
#include "partial_file_usb.hpp"
#include "partial_file_posix.hpp"

#include <mutex>

#include <buddy/storage.hpp>
#include <common/unique_file_ptr.hpp>
#include <logging/log.hpp>

LOG_COMPONENT_REF(transfers);

namespace transfers {

PartialFile::Result PartialFile::create(const char *path, size_t size) {
    unique_file_ptr file(fopen(path, "wb+"));
    if (!file) {
        return "Failed to create file";
    }
    if (storage::type(path) == storage::Type::usb) {
        return UsbPartialFile::create(path, std::move(file), size);
    }
    return PosixPartialFile::create(path, std::move(file), size);
}

PartialFile::Result PartialFile::open(const char *path, State state) {
    // File modes are explained at the declaration.
    if (storage::type(path) == storage::Type::usb) {
        unique_file_ptr file(fopen(path, "rb"));
        if (!file) {
            return "Failed to open file";
        }
        return UsbPartialFile::open(path, std::move(file), state);
    }
    unique_file_ptr file(fopen(path, "rb+"));
    if (!file) {
        return "Failed to open file";
    }
    return PosixPartialFile::open(path, std::move(file), state);
}

PartialFile::State PartialFile::get_state() const {
    std::unique_lock lock(state_mutex);
    return state;
}

bool PartialFile::has_valid_head(size_t bytes) const {
    auto s = get_state();
    return s.valid_head && s.valid_head->start == 0 && s.valid_head->end >= bytes;
}

bool PartialFile::has_valid_tail(size_t bytes) const {
    auto s = get_state();
    return s.valid_tail && s.valid_tail->start <= (s.total_size - bytes) && s.valid_tail->end == s.total_size;
}

void PartialFile::log_progress() {
    const int percent = get_state().get_percent_valid();
    if (percent != last_progress_percent) {
        last_progress_percent = percent;
        log_info(transfers, "Transfer progress: %d%%", percent);
    }
}

void PartialFile::notify_written() {
    if (written_callback) {
        written_callback(written_callback_arg);
    }
}

void PartialFile::extend_valid_part(ValidPart new_part) {
    std::unique_lock lock(state_mutex);

    // Extend the head — either grow an existing head or seed it from offset 0.
    if (state.valid_head) {
        state.valid_head->merge(new_part);
    } else if (new_part.start == 0) {
        state.valid_head = new_part;
    }
    const size_t head_end = state.valid_head ? state.valid_head->end : 0;

    // Extend the tail — either grow an existing tail or seed it past the head.
    if (state.valid_tail) {
        state.valid_tail->merge(new_part);
    } else if (new_part.start > head_end) {
        state.valid_tail = new_part;
    }

    // If the head reached the end of the file, mirror it into the tail so
    // `has_valid_tail()` returns true for downloads that filled sequentially.
    if (state.valid_head && state.valid_head->end == state.total_size) {
        state.valid_tail = state.valid_head;
    }

    // If head and tail met, merge them into the same range.
    if (state.valid_head && state.valid_tail) {
        state.valid_head->merge(*state.valid_tail);
        state.valid_tail->merge(*state.valid_head);
    }
}

} // namespace transfers
