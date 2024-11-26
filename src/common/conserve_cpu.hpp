#pragma once

#include <atomic>
#include <cstdint>

namespace buddy {

class ConserveCpu;

ConserveCpu &conserve_cpu();

/// Request for other systems to please limit their CPU usage.
///
/// Unlike priorities, this can be used for stuff that live somewhere "in the
/// middle" of a task, instead of a whole task. Currently, we limit the rolling
/// text on the screen while still keeping the GUI responsive.
class ConserveCpu {
private:
    // Number of current requests
    //
    // (multiple systems can request the CPU limiting at the same time and each
    // one need to withdraw its request independently).
    std::atomic<uint32_t> requests = 0;

public:
    // RAII of a request to limit CPU
    class Guard {
    private:
        ConserveCpu &owner;

    public:
        Guard(ConserveCpu &owner = conserve_cpu())
            : owner(owner) {
            owner.requests.fetch_add(1);
        }
        Guard(const Guard &other) = delete;
        Guard(Guard &&other) = delete;
        Guard &operator=(const Guard &other) = delete;
        Guard &operator=(Guard &&other) = delete;
        ~Guard() {
            owner.requests.fetch_sub(1);
        }
    };

    // Are there any requests to limit CPU right now?
    bool is_requested() const {
        return requests.load() > 0;
    }
};

} // namespace buddy
