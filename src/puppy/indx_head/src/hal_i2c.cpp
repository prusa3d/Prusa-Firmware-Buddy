#include "hal.hpp"
#include "critical_section.hpp"
#include "rtt.hpp"
#include <filters/debouncer.hpp>

#include <bsod/bsod.h>
#include <fpm/math.hpp>
#include <lp5817/lp5817.hpp>
#include <freertos/binary_semaphore.hpp>
#include <freertos/mutex.hpp>
#include <freertos/timing.hpp>
#include <raii/lock_guard.hpp>
#include <raii/scope_guard.hpp>
#include <stm32c0xx_hal.h>
#include <tpis/tpis.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <optional>
#include <ranges>
#include <utils/byte_utils.hpp>

namespace hal::peripherals {
extern I2C_HandleTypeDef hi2c1;
}

namespace hal::i2c {
namespace {

    class I2cBus1 {
    public:
        [[nodiscard]] bool write_memory(::i2c::Address address, uint8_t offset, Bytes tx_buf) {
            return execute_i2c_operation(address, [&]() {
                uint8_t *data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(tx_buf.data()));
                return HAL_I2C_Mem_Write_DMA(&peripherals::hi2c1, (address << 1) | write_flag, offset, I2C_MEMADD_SIZE_8BIT, data, tx_buf.size());
            });
        }

        [[nodiscard]] bool read_memory(::i2c::Address address, uint8_t offset, WritableBytes rx_buf) {
            return execute_i2c_operation(address, [&]() {
                uint8_t *data = reinterpret_cast<uint8_t *>(rx_buf.data());
                return HAL_I2C_Mem_Read_DMA(&peripherals::hi2c1, (address << 1) | write_flag, offset, I2C_MEMADD_SIZE_8BIT, data, rx_buf.size());
            });
        }

        [[nodiscard]] bool raw_transmit(::i2c::Address address, Bytes tx_buf) {
            return execute_i2c_operation(address, [&]() {
                uint8_t *data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(tx_buf.data()));
                return HAL_I2C_Master_Transmit_DMA(&peripherals::hi2c1, (address << 1) | write_flag, data, tx_buf.size());
            });
        }

        [[nodiscard]] bool raw_receive(::i2c::Address address, WritableBytes rx_buf) {
            return execute_i2c_operation(address, [&]() {
                uint8_t *data = reinterpret_cast<uint8_t *>(rx_buf.data());
                return HAL_I2C_Master_Receive_DMA(&peripherals::hi2c1, (address << 1) | write_flag, data, rx_buf.size());
            });
        }

        void delay_us(uint32_t us) {
            uint32_t ms = us / 1'000;
            ms += (us - ms * 1'000) > 0;
            freertos::delay(ms + 1); // ensures at least 'ms' miliseconds wait
        }

        static void finish_i2c_transfer([[maybe_unused]] I2C_HandleTypeDef *hi2c) {
            debug_assert(hi2c == &peripherals::hi2c1);
            if (waiting_for_i2c.exchange(false)) {
                i2c_it_semaphore.release_from_isr();
            }
        }

        static void handle_i2c_error([[maybe_unused]] I2C_HandleTypeDef *hi2c) {
            i2c_error_flag.store(true);
            finish_i2c_transfer(hi2c);
        }

    private:
        static constexpr uint8_t write_flag = 0;

        inline static freertos::Mutex i2c_mutex {};
        inline static freertos::BinarySemaphore i2c_it_semaphore {};
        inline static std::atomic<bool> i2c_error_flag { false };
        inline static std::atomic<bool> waiting_for_i2c { false }; // Track if we're waiting for I2C completion

        template <typename Op>
        [[nodiscard]] bool execute_i2c_operation(::i2c::Address address, Op op) {
            LockGuard lg { i2c_mutex };
            i2c_error_flag.store(false);
            waiting_for_i2c.store(true);
            const auto ret = op();
            if (ret != HAL_OK) {
                waiting_for_i2c.store(false);
                i2c_recover();
                return false;
            }
            static constexpr uint32_t I2C_TIMEOUT_MS = 50;
            if (!i2c_it_semaphore.try_acquire_for(I2C_TIMEOUT_MS)) {
                waiting_for_i2c.store(false);
                HAL_I2C_Master_Abort_IT(&peripherals::hi2c1, (address << 1) | write_flag);
                i2c_recover();
                return false;
            }
            if (i2c_error_flag.load()) {
                i2c_recover();
                return false;
            }
            return true;
        }

        /// Attempts to recover I2C bus after an error
        void i2c_recover() {
            rtt::print("i2c: recover\n");
            // Reset I2C peripheral
            __HAL_I2C_DISABLE(&peripherals::hi2c1);
            // Small delay to allow bus to settle
            freertos::delay(1);
            __HAL_I2C_ENABLE(&peripherals::hi2c1);
            i2c_error_flag.store(false);
        }
    };

    namespace thermometer {
        tpis::Tpis<I2cBus1> tpis_sensor;

        void init() {
            constexpr int retries = 5;
            for (int i = 0; i < retries; i++) {
                const auto result = tpis_sensor.init();
                if (result.has_value()) {
                    return;
                }
                rtt::print("i2c: thermo init failed\n");
                constexpr int wait_ms = 100;
                freertos::delay(wait_ms);
            }
        }
    } // namespace thermometer

    namespace leds {

        struct SetLEDDelayed {
            uint8_t r, g, b;
        };
        std::atomic<std::optional<SetLEDDelayed>> set_pwm_delayed { std::nullopt };
        std::atomic<std::optional<uint16_t>> set_fade_delayed { std::nullopt };

        using Controller = lp5817::LP5817<I2cBus1>;
        Controller controller;

        void init() {
            if (const auto res = controller.init(); !res.has_value()) {
                rtt::print("i2c: leds init failed");
            }
        }

        static constexpr uint8_t gamma_float(uint8_t in) {
            static constexpr float gamma = 2.2f; // Use 2.6 or 2.8 for richer colors
            static constexpr float rgb_max = 255.f;
            static constexpr float pwm_max = 255.f;
            return static_cast<uint8_t>(pwm_max * std::pow(float(in) / rgb_max, gamma));
        }

        static auto gamma_int_lut = []() consteval {
            std::array<uint8_t, 256> res {};
            const auto src = std::views::iota(0) | std::views::transform(gamma_float);
            std::ranges::copy_n(src.begin(), res.size(), res.begin());
            return res;
        }();

    } // namespace leds

} // namespace

void init_comm() {
    thermometer::init();
    leds::init();
}

CheckedTemperatureReading read_tpis_temperature() {
    static tpis::TemperatureReading last_reading {
        .object_temperature_celsius = tpis::fixed(25),
        .ambient_temperature_celsius = tpis::fixed(25),
    };

    // A jump > 50°C between consecutive readings is physically impossible
    static constexpr int32_t max_plausible_jump = 50; // °C, integer compare
    // Accept after 3 consecutive implausible readings
    static Debouncer<bool> temp_debouncer { false, 3 };

    const auto temps = thermometer::tpis_sensor.get_temps();
    if (!temps.has_value()) {
        rtt::print("i2c: thermo read failed\n");
        // Return last valid temperature on error
        return { .temps = last_reading, .valid = false };
    }
    const int32_t diff = static_cast<int32_t>(temps->object_temperature_celsius) - static_cast<int32_t>(last_reading.object_temperature_celsius);
    const bool plausible = (diff > -max_plausible_jump) && (diff < max_plausible_jump);
    temp_debouncer.push(plausible);

    if (plausible) {
        last_reading = temps.value();
        return { .temps = temps.value(), .valid = true };
    }

    if (temp_debouncer.is_stable() && !temp_debouncer.value()) {
        last_reading = temps.value();
        return { .temps = temps.value(), .valid = true };
    }

    // Transient glitch — return last known good value
    return { .temps = last_reading, .valid = true };
}

void set_led_pwm(uint8_t r, uint8_t g, uint8_t b) {
    const auto pwm = leds::SetLEDDelayed { .r = leds::gamma_int_lut.at(r), .g = leds::gamma_int_lut.at(g), .b = leds::gamma_int_lut.at(b) };
    leds::set_pwm_delayed.store(pwm);
}

void set_led_fade(uint16_t interval_ms) {
    leds::set_fade_delayed.store(interval_ms);
}

void trigger_delayed_request() {
    const auto fade = leds::set_fade_delayed.exchange(std::nullopt);
    if (fade.has_value()) {
        const auto fade_time = leds::Controller::nearest_fade_time(*fade);
        const auto enable_fade = (*fade != 0);

        if (const auto res = leds::controller.set_fade_time(fade_time, enable_fade, enable_fade, enable_fade); !res.has_value()) {
            rtt::print("i2c: delayed leds fade failed");
        }

        return; // One request at a time
    }

    const auto pwm = leds::set_pwm_delayed.exchange(std::nullopt);
    if (pwm.has_value()) {
        if (const auto res = leds::controller.set_color(pwm->r, pwm->g, pwm->b); !res.has_value()) {
            rtt::print("i2c: delayed leds failed");
        }

        return; // One request at a time
    }
}

} // namespace hal::i2c

extern "C" void HAL_I2C_MasterTxCpltCallback([[maybe_unused]] I2C_HandleTypeDef *hi2c) {
    hal::i2c::I2cBus1::finish_i2c_transfer(hi2c);
}

extern "C" void HAL_I2C_MasterRxCpltCallback([[maybe_unused]] I2C_HandleTypeDef *hi2c) {
    hal::i2c::I2cBus1::finish_i2c_transfer(hi2c);
}

extern "C" void HAL_I2C_MemTxCpltCallback([[maybe_unused]] I2C_HandleTypeDef *hi2c) {
    hal::i2c::I2cBus1::finish_i2c_transfer(hi2c);
}

extern "C" void HAL_I2C_MemRxCpltCallback([[maybe_unused]] I2C_HandleTypeDef *hi2c) {
    hal::i2c::I2cBus1::finish_i2c_transfer(hi2c);
}

extern "C" void HAL_I2C_ErrorCallback([[maybe_unused]] I2C_HandleTypeDef *hi2c) {
    hal::i2c::I2cBus1::handle_i2c_error(hi2c);
}
