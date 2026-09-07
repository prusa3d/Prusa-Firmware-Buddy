#pragma once

#include <array>

#include <pwm_utils.hpp>
#include <freertos/mutex.hpp>
#include <general_response.hpp>
#include <utils/timing/rate_limiter.hpp>
#include <utils/tristate.hpp>

#include "chamber_filtration_enums.hpp"

namespace buddy {

/// API for controlling chamber filtration (filtering the fumes out during & after print)
/// The API is thread-safe
class ChamberFiltration {

public:
    static constexpr size_t max_backend_count = 4;
    static constexpr size_t max_post_print_filtration_time_min = 90; // Propagate any change to Connect team

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

    void set_backend(ChamberFiltrationBackend backend);

    bool is_enabled() const {
        return backend() != ChamberFiltrationBackend::none;
    }

    void step();

    /// \brief Forces/disables the filtration for the current print
    /// \param set Tristate::yes if the filtration should be forced on (ignore filament needs), Tristate::no if it should be forced off. Tristate::other will disable the override.
    void set_needs_filtration_override(Tristate set);

public:
    /// \returns rated lifetime of the HEPA filter in seconds. 0 if not unknown/not specified
    uint32_t filter_lifetime_s() const;

    /// Check HEPA filter expiration and possibly show warning
    void check_filter_expiration();

    /// Process filter change, reset timers and such
    void change_filter();

    /// Processes response from WarningType::EnclosureFilterExpiration
    void handle_filter_expiration_warning(Response response);

private:
    bool needs_filtration() const;

private:
    mutable freertos::Mutex mutex_;

    PWM255 output_pwm_;

    /// A print can explicitly request that it wants/doesn't want filtration
    Tristate needs_filtration_override_ = Tristate::other;

    /// ticks_s() of the time where we last needed the filtration (were printing)
    std::optional<uint32_t> last_filtration_need_s_ = std::nullopt;

    /// ticks_s() of the start of filter usage (output_pwm > 0) that has not yet been emitted in the config_store
    uint32_t unaccounted_filter_time_used_start_s_ = 0;

    /// We don't need to run step very often
    RateLimiter<uint32_t> step_rate_limiter_s_ { 2 };
};

ChamberFiltration &chamber_filtration();

} // namespace buddy
