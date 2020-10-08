#pragma once
namespace buddy::hw {

struct Pin {
    enum class State {
        low,
        high
    };
};

struct jogPin {
    Pin::State state;
    auto read() const { return state; }
};

extern jogPin jogWheelEN1;
extern jogPin jogWheelEN2;
extern jogPin jogWheelENC;

} // namespace buddy::hw
