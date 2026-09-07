#include "tracker.hpp"

#include <random/random.h>
#include <utils/string_builder.hpp>

#include <algorithm>
#include <mutex>
#include <cinttypes>
#include <bsod/bsod.h>

namespace buddy::cork {

std::optional<Tracker::Slot> *Tracker::get_slot(Cookie cookie) {
    const size_t i = idx(cookie);
    debug_assert(i <= slots.size());
    if (slots[i].has_value() && slots[i]->cookie == cookie) {
        return &slots[i];
    } else {
        return nullptr;
    }
}

std::optional<Tracker::CorkHandle> Tracker::new_cork() {
    std::unique_lock lock(mutex);

    auto slot = std::find_if(slots.begin(), slots.end(), [](auto &slot) { return !slot.has_value(); });
    if (slot == slots.end()) {
        return std::nullopt;
    }

    size_t i = slot - slots.begin();

    // Avoid collisions by incorporating the index into the cookie.
    Cookie c = (rand_u() & ~slot_mask) | (i & slot_mask);
    *slot = {
        .done = false,
        .cookie = c,
    };
    return CorkHandle(c, this);
}

void Tracker::clear() {
    std::unique_lock lock(mutex);
    for (auto &slot : slots) {
        if (slot.has_value()) {
            slot->done = true;
        }
    }

    clears_called.fetch_add(1);
}

void Tracker::mark_done(Cookie cookie) {
    // Note:
    // We are not using the get_slot here, because that one assumes the cookie
    // is correct (and asserts that eg. the index falls within the range).
    // Here, the cookie may come from outside world (gcode submitted by user,
    // which is not intended, but we can't really prevent it reasonably), so we
    // act differently and silently discard it instead of error on it.
    size_t i = idx(cookie);

    if (i >= slots.size()) {
        return;
    }

    std::unique_lock lock(mutex);

    auto &slot = slots[i];

    if (slot.has_value() && slot->cookie == cookie) {
        slot->done = true;
        return;
    }
}

Tracker::CorkHandle::CorkHandle(CorkHandle &&other)
    : cookie(other.cookie)
    , owner(other.owner) {
    other.owner = nullptr;
}

Tracker::CorkHandle &Tracker::CorkHandle::operator=(CorkHandle &&other) {
    destroy();
    cookie = other.cookie;
    owner = other.owner;
    other.owner = nullptr;
    return *this;
}

Tracker::CorkHandle::~CorkHandle() {
    destroy();
}

void Tracker::CorkHandle::destroy() {
    if (owner != nullptr) {
        std::unique_lock lock(owner->mutex);

        if (auto *slot = owner->get_slot(cookie); slot != nullptr) {
            slot->reset();
        }
    }
}

bool Tracker::CorkHandle::is_done_and_consumed() {
    if (owner != nullptr) {
        std::unique_lock lock(owner->mutex);

        if (auto slot = owner->get_slot(cookie); slot != nullptr) {
            const bool done = (*slot)->done;
            if (done) {
                slot->reset();
                owner = nullptr;
            }
            return done;
        }
    }

    debug_assert(false /* Dangling or already consumed handle */);
    return true;
}

std::array<char, 20> Tracker::CorkHandle::get_gcode() const {
    std::array<char, 20> buffer;
    StringBuilder builder(buffer);
    builder.append_printf("M9933 C%" PRIu16, get_cookie());
    return buffer;
}

uint32_t Tracker::clear_cnt() const {
    return clears_called.load();
}

Tracker tracker;

} // namespace buddy::cork
