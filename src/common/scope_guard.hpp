#pragma once

#include <utility>

/// A RAII adaptor for a clean-up closure.
///
/// Captures a closure to be called at the end of scope. If more are used, they
/// get called in reverse order.
template <class Cback>
class [[nodiscard]] ScopeGuard {
private:
    Cback cback;
    bool armed = true;

public:
    ScopeGuard(Cback &&cback, bool arm = true)
        : cback(std::move(cback))
        , armed(arm) {}
    ScopeGuard(const ScopeGuard &other) = delete;
    ScopeGuard(ScopeGuard &&other) = delete;
    ScopeGuard &operator=(const ScopeGuard &other) = delete;
    ScopeGuard &operator=(ScopeGuard &&other) = delete;
    ~ScopeGuard() {
        if (armed) {
            cback();
        }
    }
    /// Cancel calling of the closure.
    void disarm() { armed = false; }
};
