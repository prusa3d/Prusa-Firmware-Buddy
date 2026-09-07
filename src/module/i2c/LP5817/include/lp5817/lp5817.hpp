#pragma once

#include <i2c/base.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <iterator>
#include <utility>

namespace lp5817 {
template <i2c::I2cBus HWImpl>
class LP5817 : public HWImpl {
public:
    static constexpr i2c::Address ADDRESS = 0x2D;

    enum class Error {
        hal_error, // HWImpl function returned false
        brownout, // POR reported in flags
        overheat, // TSD reported in flags
        not_inited, // The state machine was not initialized
    };

    /// Predefined values that chip allowes to set the fade time to.
    ///
    /// The values are represented in seconds so:
    /// * s1 = 1 second
    /// * s0_1 = 0.1 seconds
    enum class FadeTime : uint16_t {
        s0 = 0x0,
        s0_05 = 0x1,
        s0_1 = 0x2,
        s0_15 = 0x3,
        s0_2 = 0x4,
        s0_25 = 0x5,
        s0_3 = 0x6,
        s0_35 = 0x7,
        s0_4 = 0x8,
        s0_45 = 0x9,
        s0_5 = 0xa,
        s1 = 0xb,
        s2 = 0xc,
        s4 = 0xd,
        s6 = 0xe,
        s8 = 0xf
    };

    /// Number of FadeTime values (s8 is the last enumerator).
    static constexpr size_t fade_time_count = static_cast<size_t>(std::to_underlying(FadeTime::s8)) + 1;

    /// Duration in milliseconds of each FadeTime, indexed by its underlying value. Ascending.
    /// Too many initializers is a hard error; too few leaves a 0 that breaks the is_sorted check below.
    static constexpr std::array<uint16_t, fade_time_count> fade_time_ms {
        0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500,
        1000, 2000, 4000, 6000, 8000
    };
    static_assert(std::ranges::is_sorted(fade_time_ms));

    /// Returns the FadeTime whose duration is closest to `ms`.
    /// Ties (exact midpoints) resolve to the shorter fade.
    static constexpr FadeTime nearest_fade_time(uint32_t ms) {
        // Table is sorted ascending: binary-search the first entry >= ms, then
        // compare it against its predecessor and keep whichever is closer.
        const auto upper = std::lower_bound(fade_time_ms.begin(), fade_time_ms.end(), ms);
        if (upper == fade_time_ms.begin()) {
            return FadeTime::s0; // ms <= shortest
        }
        if (upper == fade_time_ms.end()) {
            return FadeTime::s8; // ms >= longest
        }
        const auto lower = upper - 1;
        const uint32_t diff_lower = ms - *lower;
        const uint32_t diff_upper = *upper - ms;
        // <= keeps the shorter fade (lower) on an exact midpoint tie.
        const auto chosen = (diff_lower <= diff_upper) ? lower : upper;
        return static_cast<FadeTime>(std::distance(fade_time_ms.begin(), chosen));
    }
    static_assert(nearest_fade_time(0) == FadeTime::s0);
    static_assert(nearest_fade_time(1000) == FadeTime::s1);
    static_assert(nearest_fade_time(120) == FadeTime::s0_1); // 120 -> 100 (rounds down)
    static_assert(nearest_fade_time(125) == FadeTime::s0_1); // midpoint 100/150 -> shorter
    static_assert(nearest_fade_time(126) == FadeTime::s0_15); // 126 -> 150 (rounds up)
    static_assert(nearest_fade_time(3000) == FadeTime::s2); // midpoint 2000/4000 -> shorter
    static_assert(nearest_fade_time(999999) == FadeTime::s8); // saturates at max

    template <typename T>
    using Result = std::expected<T, Error>;

    // TODO: More configuration arguments (aka power and initial color)
    Result<void> init() {
        // --- STEP 0: Software Reset ---
        // Ensure chip is in a clean state (Datasheet 7.6.7) [cite: 982]
        if (const auto res = write_register(Register::RESET, 0xCC); !res.has_value()) {
            return res;
        }
        this->delay_us(5'000); // Wait for internal reboot
        // --- STEP 1: Enable Chip ---
        // (Write 01h to register 00h)
        if (const auto res = write_register(Register::CHIP_EN, 0x01); !res.has_value()) {
            return res;
        }

        // Datasheet requirement: Wait >1ms after enable before analog config [cite: 1138]
        this->delay_us(1'000);

        state = State::initializing;

        // Extra non documented step. After enable we should clear the POWER ON flag
        if (const auto res = write_register(Register::FLAG_CLR, 0x01); !res.has_value()) {
            return res;
        }

        // --- STEP 2: Configure Current (51mA range) ---
        // (Write 01h to register 01h)
        if (const auto res = write_register(Register::CONF0, 0x01); !res.has_value()) {
            return res;
        }

        // --- STEP 3: Set Dot Currents (Analog Gain) ---
        // Setting current for all channels
        // These values are tuned for INDX head, if any other device will use this chip, make them a configurable values
        static constexpr std::array<std::byte, 3> target_pwr = { std::byte { 0xff }, std::byte { 0x4a }, std::byte { 0x68 } };
        if (!this->write_memory(ADDRESS, std::to_underlying(Register::OUT0_DC), target_pwr)) {
            state = State::invalid;
            return std::unexpected(Error::hal_error);
        }

        // --- STEP 4: Enable Outputs ---
        // Enable OUT0, OUT1, OUT2 (Binary 0000 0111)
        if (const auto res = write_register(Register::CONF1, 0x07); !res.has_value()) {
            return res;
        }

        // -- Extra STEP --
        // Disable fade by default
        if (const auto res = set_fade_time(FadeTime::s0, false, false, false); !res.has_value()) {
            return res;
        }

        // --- STEP 5: Apply Configuration (UPDATE 1) ---
        // Essential to latch analog configurations [cite: 887]
        // No action needed: set_fade_time() above already issued the Update command.

        // --- STEP 6: Set Initial PWM (Brightness) ---
        if (const auto res = set_color(0xff, 0xff, 0xff); !res.has_value()) {
            return res;
        }

        // Wait for analog circuit to stabilize before checking faults
        this->delay_us(5'000);

        // --- STEP 7: Fault Verification ---
        if (const auto res = check_for_faults(); !res.has_value()) {
            return res;
        }

        state = State::running;

        return {};
    }

    Result<void> check_for_faults() {
        Result<uint8_t> faults = 0;
        if (faults = read_register(Register::FLAG); !faults.has_value()) {
            return std::unexpected(faults.error());
        }

        static constexpr uint8_t POR_FLAG = 0b0000'0001;
        if (*faults & POR_FLAG) {
            return std::unexpected(Error::brownout);
        }

        static constexpr uint8_t TSD_FLAG = 0b0000'0010;
        if (*faults & TSD_FLAG) {
            return std::unexpected(Error::overheat);
        }

        return {};
    }

    Result<void> set_color(uint8_t r, uint8_t g, uint8_t b) {
        if (state != State::running && state != State::initializing) {
            state = State::invalid;
            return std::unexpected(Error::not_inited);
        }

        const std::array<std::byte, 3> desired_color = { std::byte { r }, std::byte { g }, std::byte { b } };

        if (!this->write_memory(ADDRESS, std::to_underlying(Register::OUT0_PWM), desired_color)) {
            state = State::invalid;
            return std::unexpected(Error::hal_error);
        }

        return {};
    }

    Result<void> set_fade_time(FadeTime fade, bool en_ch0 = true, bool en_ch1 = true, bool en_ch2 = true) {
        if (state != State::running && state != State::initializing) {
            state = State::invalid;
            return std::unexpected(Error::not_inited);
        }

        const uint8_t value = (std::to_underlying(fade) << 4) | (static_cast<uint8_t>(en_ch2) << 2) | (static_cast<uint8_t>(en_ch1) << 1) | (static_cast<uint8_t>(en_ch0) << 0);

        if (const auto res = write_register(Register::CONF2, value); !res.has_value()) {
            return res;
        }
        // DEV_CONFIG registers only latch on an Update command (datasheet Table 7-5).
        return commit_config();
    }

protected:
    enum class Register : uint8_t {
        CHIP_EN = 0x00,

        CONF0 = 0x01,
        CONF1 = 0x02,
        CONF2 = 0x03,
        CONF3 = 0x04,

        SHUTDOWN = 0x0D,
        RESET = 0x0E,
        UPDATE = 0x0F,

        FLAG_CLR = 0x13,

        OUT0_DC = 0x14,
        OUT1_DC = 0x15,
        OUT2_DC = 0x16,

        OUT0_PWM = 0x18,
        OUT1_PWM = 0x19,
        OUT2_PWM = 0x1A,

        FLAG = 0x40,
    };

    Result<void> write_register(Register reg, uint8_t value) {
        if (!this->write_memory(ADDRESS, std::to_underlying(reg), std::span { reinterpret_cast<const std::byte *>(&value), 1 })) {
            state = State::invalid;
            return std::unexpected(Error::hal_error);
        }
        return {};
    }

    Result<uint8_t> read_register(Register reg) {
        uint8_t value = 0;
        if (!this->read_memory(ADDRESS, std::to_underlying(reg), std::span { reinterpret_cast<std::byte *>(&value), 1 })) {
            state = State::invalid;
            return std::unexpected(Error::hal_error);
        }
        return { value };
    }

    /// Latches pending DEV_CONFIG0..3 (0x01-0x04) writes; requires CHIP_EN = 1.
    Result<void> commit_config() {
        return write_register(Register::UPDATE, 0x55);
    }

    enum class State : uint8_t {
        invalid,
        initializing,
        running,
    } state
        = State::invalid;
};
} // namespace lp5817
