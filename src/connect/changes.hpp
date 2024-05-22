#pragma once

#include <array>
#include <cstdint>

namespace connect_client {

class Tracked {
private:
    uint32_t hash = 0;
    bool dirty = true;

public:
    bool is_dirty() const {
        return dirty;
    }

    void mark_clean() {
        dirty = false;
    }

    void mark_dirty() {
        dirty = true;
    }

    bool set_hash(uint32_t new_hash) {
        if (new_hash != hash) {
            mark_dirty();
            hash = new_hash;
        }

        return is_dirty();
    }
};

} // namespace connect_client
