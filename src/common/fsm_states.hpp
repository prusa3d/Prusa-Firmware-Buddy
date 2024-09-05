#pragma once

#include "client_fsm_types.h"
#include <array>
#include <common/fsm_base_types.hpp>
#include <optional>

namespace fsm {

class States {
public:
    // TODO In the future, we could ditch the optional here.
    //      Instead, each fsm would have 'inactive' phase (some of them already do).
    using State = std::optional<BaseData>;

private:
    std::array<State, static_cast<size_t>(ClientFSM::_count)> states;

public:
    uint32_t generation = 0;

    constexpr bool is_active(ClientFSM client_fsm) const {
        return (*this)[client_fsm].has_value();
    }

    constexpr State &operator[](ClientFSM client_fsm) {
        return states[static_cast<size_t>(client_fsm)];
    }

    constexpr const State &operator[](ClientFSM client_fsm) const {
        return states[static_cast<size_t>(client_fsm)];
    }

    struct Top {
        ClientFSM fsm_type;
        BaseData data;

        bool operator==(const Top &) const = default;
        bool operator!=(const Top &) const = default;
    };
    std::optional<Top> get_top() const;

    void log() const;
};

} // namespace fsm
