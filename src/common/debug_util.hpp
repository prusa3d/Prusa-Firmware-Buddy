#pragma once

#include <tuple>
#include <cstdint>
#include "traits.hpp"
#include "timing.h"
#include <Pin.hpp>

/**
 * Invoke a given function only when its arguments have changed or when given
 * time elapsed.
 *
 * Typical usage:
 * static RateLimited algorithm_reporter(100, [&](int a, int b) {
 *        log_debug(MyComponent, "Val: %d, %d", a, b);
 *     });
 * algorithm_reporter(resul1, result2);
 **/
template <typename F>
class RateLimited {
    uint32_t _last_ping = 0;

    int _rate_limit_ms;
    F _job;
    function_traits<F>::argument_types _args;

public:
    RateLimited(int rate_limit, F f)
        : _rate_limit_ms(rate_limit)
        , _job(f) {}

    template <typename... Args>
    void operator()(Args... args) {
        uint32_t now = ticks_ms();
        auto args_tuple = typename function_traits<F>::argument_types(args...);
        if (args_tuple != _args || ticks_diff(now, _last_ping) > _rate_limit_ms) {
            _job(args...);
            _args = args_tuple;
            _last_ping = now;
        }
    }
};

struct TrackTimingViaPin {
    buddy::hw::OutputPin _pin;
    bool _stopped = false;

    TrackTimingViaPin(auto pin)
        : _pin(pin) {
        _pin.write(buddy::hw::Pin::State::high);
    }

    void stop() {
        if (_stopped) {
            return;
        }
        _pin.write(buddy::hw::Pin::State::low);
        _stopped = true;
    }

    ~TrackTimingViaPin() {
        stop();
    }
};
