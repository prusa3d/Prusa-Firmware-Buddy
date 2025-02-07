#pragma once

#include <array>

#include <pwm_utils.hpp>
#include <freertos/mutex.hpp>

#include "chamber_filtration_enums.hpp"

namespace buddy {

/// API for controlling chamber filtration (filtering the fumes out during & after print)
/// The API is thread-safe
class ChamberFiltration {

public:
    static constexpr size_t max_backend_count = 4;

    using Backend = ChamberFiltrationBackend;
    using BackendArray = std::array<Backend, max_backend_count>;

    /// \returns translatable name of the provided filtration backend
    static const char *backend_name(Backend backend);

    /// Stores all available backends in an UI-friendly manner into the target memory (including "none")
    /// \returns number of backends
    static size_t get_available_backends(BackendArray &target);

public:
    /// \returns PWM the filtration fan should be driven with
    /// This is the minimum PWM the fan should be running at - if the fan serves some other purpose as well, you can use std::max
    PWM255 output_pwm() const;

    /// \returns the current backend that should be using the filtration API
    ChamberFiltrationBackend backend() const;

    void step();

private:
    void update_needs_filtration();

private:
    mutable freertos::Mutex mutex_;

    PWM255 output_pwm_;
    bool is_printing_prev_ = false;
    bool needs_filtration_ = false;
    uint32_t last_print_s_ = 0;
};

ChamberFiltration &chamber_filtration();

} // namespace buddy
