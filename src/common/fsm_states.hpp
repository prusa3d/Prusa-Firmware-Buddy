#pragma once

#include "client_fsm_types.hpp"
#include <array>
#include <common/fsm_base_types.hpp>
#include <optional>
#include <utils/custom_uint31_t.hpp>

namespace fsm {

class States {
public:
    // TODO In the future, we could ditch the optional here.
    //      Instead, each fsm would have 'inactive' phase (some of them already do).
    using State = std::optional<BaseData>;

private:
    std::array<State, static_cast<size_t>(ClientFSM::_count)> states;

protected:
    StateId state_id = 0; // unique ID of the state instance

public:
    States() = default;

    // Disallow copy construction, states are big!
    States(const States &) = delete;
    States(States &&) = delete;

    States &operator=(const States &) = default;

public:
    inline void init_state_id() {
        state_id = StateId::generate_random_uint31();
    }

    constexpr void increment_state_id() {
        state_id++;
    }

    inline StateId get_state_id() const {
        return state_id;
    }

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
