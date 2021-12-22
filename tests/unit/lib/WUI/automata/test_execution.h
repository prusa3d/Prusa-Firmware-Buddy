#pragma once

#include <automata/core.h>

#include <vector>

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
        : Execution(automaton) {}
};

}
