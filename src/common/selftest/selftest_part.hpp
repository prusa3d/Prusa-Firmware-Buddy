/**
 * @file selftest_part.hpp
 * @author Radek Vana
 * @brief Selftest state machine template
 * @date 2021-09-24
 */
#pragma once
#include <cstdint>
#include <array>
#include <functional>
#include "i_selftest_part.hpp"
#include "marlin_server.hpp"

namespace selftest {

/**
 * @brief Selftest state machine able to hold any number of states, without dynamic allocation
 * there is an use example in cpp file
 *
 * @tparam T selftest part class type, it must have compatible interface
 * @tparam SZ number of states - number of state member functions passed to ctor
 */
template <class T, class CNF, size_t SZ>
class PartHandler : public IPartHandler {
public:
    // CNF::type is depended name and would be assumed not type without typename keyword
    using EvaluationType = typename CNF::type_evaluation;
    using state_fnc = LoopResult (T::*)();
    using state_hook_fnc = void (*)(T &);

private:
    T instance;
    std::array<state_fnc, SZ> arr;

    EvaluationType &refResult;
    state_hook_fnc fnc_state_changed;
    state_hook_fnc fnc_state_remained;

    virtual void hook_state_changed() override {
        fnc_state_changed(instance);
    }

    virtual void hook_remained_in_state() override {
        fnc_state_remained(instance);
    }

    virtual LoopResult invokeCurrentState() override {
        // index was tested by parent
        auto fn = arr[this->currentState()];
        return std::invoke(fn, instance);
    }
    virtual Response processButton() override {
        const auto phase_enum = GetFsmPhase();
        const Response response = marlin_server::get_response_from_phase(phase_enum);
        return response;
    }
    virtual void pass() override { refResult.Pass(); }
    virtual void fail() override { refResult.Fail(); }
    virtual void abort() override { refResult.Abort(); }

public:
    // ctor needs to pass reference to result and last result
    // because i need to actualize result in place, where only non template IPartHandler is known
    template <class... E>
    PartHandler(const CNF &cnf, EvaluationType &refResult_, E &&...e)
        : IPartHandler(SZ, CNF::part_type)
        , instance(*this, cnf, refResult_)
        , arr { std::forward<E>(e)... }
        , refResult(refResult_)
        , fnc_state_changed([](T &) {}) // use empty lambda for state enter, so I don't need to check nullptr
        , fnc_state_remained([](T &) {}) // use empty lambda for state remain, so I don't need to check nullptr
    {
        // result can refer to static variable, need to reset its value
        refResult = EvaluationType();
    }
    void SetStateChangedHook(state_hook_fnc f) { fnc_state_changed = f; }
    void SetStateRemainedHook(state_hook_fnc f) { fnc_state_remained = f; }
    T &GetInstance() { return instance; }
};

/**
 * @brief set of template factory methods
 *        used to create part of selftest
 */
struct Factory {
    template <class T, class CNF, class... E>
    static auto Create(const CNF &cnf,
        typename CNF::type_evaluation &refResult,
        E &&...e) {
        return PartHandler<T, CNF, sizeof...(E)>(cnf, refResult, std::forward<E>(e)...);
    }
    template <class T, class CNF, class... E>
    static constexpr auto *CreateDynamical(const CNF &cnf,
        typename CNF::type_evaluation &refResult,
        E &&...e) {
        return new PartHandler<T, CNF, sizeof...(E)>(cnf, refResult, std::forward<E>(e)...);
    }
};

}; // namespace selftest
