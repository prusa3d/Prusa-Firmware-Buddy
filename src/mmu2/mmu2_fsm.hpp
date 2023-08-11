#pragma once

#include "general_response.hpp"
#include "mmu2_reporter.hpp"

namespace MMU2 {

class Fsm {
    bool created_this = false;

    Fsm() = default;
    Fsm(const Fsm &) = delete;

public:
    void Loop();

    Response GetResponse() const;

    bool Activate();
    bool Deactivate();
    bool IsActive() const;

    static Fsm &Instance() {
        static Fsm ret;
        return ret;
    }

    Reporter reporter;
};

} // namespace MMU2
