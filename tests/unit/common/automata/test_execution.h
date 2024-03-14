#pragma once

#include <automata/core.h>

#include <vector>
#include <string>

namespace {

class TestExecution : public automata::Execution {
protected:
    virtual automata::ExecutionControl event(automata::Event event) override {
        events.push_back(event);
        return automata::ExecutionControl::Continue;
    }

public:
    std::vector<automata::Event> events;
    TestExecution(const automata::Automaton &automaton)
        : Execution(&automaton) {}
    std::string collect_entered(automata::StateIdx desired) {
        std::string result;
        for (const auto &event : events) {
            if (event.entering_state == desired) {
                result += event.payload;
            }
        }
        return result;
    }
    bool contains_enter(automata::StateIdx desired) {
        for (const auto &event : events) {
            if (event.entering_state == desired) {
                return true;
            }
        }
        return false;
    }
};

} // namespace
