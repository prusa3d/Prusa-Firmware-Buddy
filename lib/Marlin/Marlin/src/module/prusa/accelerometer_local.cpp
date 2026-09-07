#include "accelerometer.h"
#include "accelerometer_local.hpp"

#include <atomic>
#include <optional>

#include <option/has_local_accelerometer.h>
#include <printers.h>
#include <hwio_pindef.h>
#include <freertos/timing.hpp>

static_assert(HAS_LOCAL_ACCELEROMETER());

#include <lis2dh12_poller.hpp>
#include <bsod/bsod.h>

using namespace accelerometer;

namespace {
// Single-owner guard
std::atomic<bool> s_borrowed = false;
} // namespace

#if PRINTER_IS_PRUSA_MK3_5()
// MK3.5 repurposes the fan-tach pin as the accelerometer.
static std::optional<LIS2DH12Poller> g_local_accelerometer_poller;

static LIS2DH12Poller &poller() {
    return g_local_accelerometer_poller.value();
}
#else
// Boards with a dedicated CS pin run one always-on poller:
// set up once at boot and left sampling continuously
static LIS2DH12Poller &poller() {
    static LIS2DH12Poller instance { spi_handle_accelerometer, buddy::hw::acellCs, &htim9 };
    return instance;
}
#endif

// (Re)detect the accelerometer if it is not currently good.
static void ensure_setup() {
    if (poller().hw_good()) {
        return;
    }
    constexpr int RETRIES = 5;
    for (int i = 0; i < RETRIES; i++) {
        if (poller().setup_accelerometer()) {
            break;
        }
        freertos::delay(10);
    }
}

void prusa_accelerometer_local_init() {
#if PRINTER_IS_PRUSA_MK3_5()
    // No-op: the poller is created/destroyed per PrusaAccelerometer on MK3.5.
#else
    ensure_setup();
    poller().hw_start();
#endif
}

PrusaAccelerometer::PrusaAccelerometer()
#if PRINTER_IS_PRUSA_MK3_5()
    : output_enabler { buddy::hw::fanPrintTach, buddy::hw::Pin::State::high, buddy::hw::OMode::pushPull, buddy::hw::OSpeed::high }
    , output_pin { output_enabler.pin() }
#endif
{
    if (s_borrowed.exchange(true)) {
        bsod("Multiple access to local accelerometer");
    }

#if PRINTER_IS_PRUSA_MK3_5()
    // Per-session poller.
    g_local_accelerometer_poller.emplace(spi_handle_accelerometer, output_pin, &htim9);
    ensure_setup();
    poller().hw_start();
#else
    // Re-detect before session.
    ensure_setup();
#endif
    // Re-baseline for this session..
    poller().begin_session();
}

PrusaAccelerometer::~PrusaAccelerometer() {
#if PRINTER_IS_PRUSA_MK3_5()
    // Free tach pin.
    poller().stop();
    g_local_accelerometer_poller.reset();
#endif
    s_borrowed = false;
}

void PrusaAccelerometer::clear() {
    poller().clear();
}

accelerometer::Error PrusaAccelerometer::get_error() const {
    if (!poller().hw_good()) {
        return accelerometer::Error::communication;
    }
    if (poller().overflow()) {
        return accelerometer::Error::overflow_sensor;
    }
    return accelerometer::Error::none;
}

float PrusaAccelerometer::get_sampling_rate() const {
    return poller().get_sampling_rate();
}

PrusaAccelerometer::GetSampleResult PrusaAccelerometer::get_sample(RawAcceleration &raw_acceleration) {
    if (!poller().hw_good() || poller().overflow()) {
        return GetSampleResult::error;
    }

    auto sample = poller().get_sample();
    if (!sample.has_value()) {
        return GetSampleResult::buffer_empty;
    }
    raw_acceleration = *sample;

    return GetSampleResult::ok;
}

void prusa_accelerometer_handle_polling() {
    poller().polling_routine();
}

void prusa_accelerometer_handle_spi_finish() {
    poller().spi_finish_routine();
}
