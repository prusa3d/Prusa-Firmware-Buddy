#include "core.h"

#include <cassert>
#include <cctype>

using std::nullopt;
using std::optional;

namespace {

typedef bool (*SpecialCheck)(uint8_t);

bool anything(uint8_t) {
    return true;
}

bool whitespace(uint8_t byte) {
    return isspace(byte);
}

bool horiz_space(uint8_t byte) {
    return byte == ' ' or byte == '\t';
}

bool digit(uint8_t byte) {
    return isdigit(byte);
}

SpecialCheck specials[] = {
    anything,
    whitespace,
    horiz_space,
    digit,
};

} // namespace

namespace automata {

bool Transition::matches(uint8_t byte) const {
    switch (label_type) {
    case LabelType::Char:
        return byte == label;
    case LabelType::CharNoCase:
        return tolower(byte) == label;
    case LabelType::Special:
        return specials[label](byte);
    default:
        assert(0);
        return false;
    }
}

std::optional<Event> Automaton::gen_event(StateIdx old_state, StateIdx new_state, uint8_t payload) const {
    const State &o = states[old_state];
    const bool leave = o.emit_leave;
    const State &n = states[new_state];
    const bool enter = n.emit_enter;

    if (leave || enter) {
        return Event {
            old_state,
            new_state,
            leave,
            enter,
            payload,
        };
    } else {
        return nullopt;
    }
}

std::optional<TransitionResult> Automaton::transition(ActiveState old, uint8_t byte) const {
    const State &state = states[old.state];
    if (state.has_path) {
        const uint8_t converted = state.path_nocase ? tolower(byte) : byte;
        const char *path = paths[state.path].value;
        if (path[old.path] == converted) {
            if (path[old.path + 1] == '\0') {
                ActiveState new_state(old.state + 1);
                return TransitionResult {
                    new_state,
                    gen_event(old.state, new_state.state, byte),
                    false,
                };
            } else {
                return TransitionResult { old.path_step(), nullopt, false };
            }
        }
    }

    const State &sentinel = states[old.state + 1];
    for (TransIdx i = state.first_transition; i < sentinel.first_transition; i++) {
        if (transitions[i].matches(byte)) {
            const auto new_state = transitions[i].target_state;
            return TransitionResult { new_state, gen_event(old.state, new_state, byte), transitions[i].fallthrough };
        }
    }

    return nullopt;
}

ExecutionControl Execution::feed(uint8_t byte) {
    const auto new_state = automaton->transition(current_state, byte);

    if (new_state.has_value()) {
        current_state = new_state->new_state;
        if (new_state->emit_event.has_value()) {
            const auto result = event(*new_state->emit_event);
            if (result != ExecutionControl::Continue) {
                return result;
            }
        }
        if (new_state->re_feed) {
            return feed(byte);
        } else {
            return ExecutionControl::Continue;
        }
    } else {
        return ExecutionControl::NoTransition;
    }
}

std::tuple<ExecutionControl, size_t> Execution::consume(std::string_view data) {
    size_t consumed = 0;
    for (const uint8_t b : data) {
        const ExecutionControl control = feed(b);
        if (control != ExecutionControl::NoTransition) {
            consumed++;
        }
        if (control != ExecutionControl::Continue) {
            return std::make_tuple(control, consumed);
        }
    }

    return std::make_tuple(ExecutionControl::Continue, consumed);
}

} // namespace automata
