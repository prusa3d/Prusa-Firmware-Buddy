/**
 * @file neopixel.hpp
 * @author Radek Vana
 * @date 2021-05-27
 */

#pragma once
#include <stdint.h>
#include <bitset>
#include <array>
/**
 * | controller |  2811 | 2812 |  2811  |  2812  |
 * |------------|-------|------|--------|--------|
 * |   signal   | Timing [ns] || Tolerance [ns] ||
 * |  T1H [ns]  |   600 |  800 |    150 |    150 |
 * |  T1L [ns]  |   650 |  450 |    150 |    150 |
 * |  T0H [ns]  |   250 |  400 |    150 |    150 |
 * |  T0L [ns]  |  1000 |  850 |    150 |    150 |
 *
 *
 * |          | Timing [ns] | Tolerance [ns]  |
 * |controller| 2811 | 2812 |  2811  |  2812  |
 * |----------|------|------|--------|--------|
 * | T1H [ns] |  600 |  800 |    150 |    150 |
 * | T1L [ns] |  650 |  450 |    150 |    150 |
 * | T0H [ns] |  250 |  400 |    150 |    150 |
 * | T0L [ns] | 1000 |  850 |    150 |    150 |
 *
 * |          |   2811     |   2812     |
 * |          | min |  max | min |  max |
 * |----------|-----|------|-----|------|
 * | T1H [ns] | 450 |  750 | 650 |  950 |
 * | T1L [ns] | 500 |  800 | 300 |  600 |
 * | T0H [ns] | 100 |  400 | 250 |  550 |
 * | T0L [ns] | 850 | 1150 | 700 | 1000 |
 *
 * |          |   281X     |
 * |          | min |  max |
 * |----------|-----|------|
 * | T1H [ns] | 650 |  750 |
 * | T1L [ns] | 500 |  600 |
 * | T0H [ns] | 250 |  400 |
 * | T0L [ns] | 850 | 1000 |
 *
 * | SPI freq [Mhz]    | 21        |
 * | spi t [ns]        | 47.62     |
 * | range             | min | max |
 * |-------------------|-----|-----|
 * | T1H [tick]        | 14  | 15  |
 * | T1L [tick]        | 11  | 12  |
 * | T0H [tick]        | 6   | 8   |
 * | T0L [tick]        | 18  | 21  |
 * | sum log 1 [tick]  | 25  | 27  |
 * | sum log 0 [tick]  | 24  | 29  |
 *
 * |  SPI freq [Mhz]  |           10.5                               ||
 * |------------------|-----------------------------------------------|
 * | spi t [ns]       | 95.24                                        ||
 * | range            | Picked from 21 MHZ Range | Current  frequency |
 * | T1H [tick]       | 14                       | 7                  |
 * | T1L [tick]       | 12                       | 6                  |
 * | T0H [tick]       | 6                        | 3                  |
 * | T0L [tick]       | 18                       | 9                  |
 * |------------------|--------------------------|--------------------|
 * | sum log 1 [tick] | 26                       | 13                 |
 * | sum log 0 [tick] | 24                       | 12                 |
 *
 * | SPI freq [Mhz]   |            7                                 ||
 * |------------------|-----------------------------------------------|
 * | spi t [ns]       | 142.86                                       ||
 * | range            | Picked from 21 MHZ Range | Current  frequency |
 * | T1H [tick]       | 15                       | 5                  |
 * | T1L [tick]       | 12                       | 4                  |
 * | T0H [tick]       | 6                        | 2                  |
 * | T0L [tick]       | 18                       | 6                  |
 * |------------------|-----------------------------------------------|
 * | sum log 1 [tick] | 27                       | 9                  |
 * | sum log 0 [tick] | 24                       | 8                  |
 *
 */

namespace neopixel {
inline constexpr uint32_t T1H_21MHz = 14;
inline constexpr uint32_t T1L_21MHz = 11;
inline constexpr uint32_t T0H_21MHz = 6;
inline constexpr uint32_t T0L_21MHz = 18;
inline constexpr uint32_t RESET_21MHz = 51 * 21;

inline constexpr uint32_t T1H_10M5Hz = 7;
inline constexpr uint32_t T1L_10M5Hz = 6;
inline constexpr uint32_t T0H_10M5Hz = 3;
inline constexpr uint32_t T0L_10M5Hz = 9;
inline constexpr uint32_t RESET_10M5Hz = uint32_t(51.f * 10.5f);

// cannot set prescaller to 7 MHz
// and clock source is fixed
inline constexpr uint32_t T1H_7MHz = 5;
inline constexpr uint32_t T1L_7MHz = 4;
inline constexpr uint32_t T0H_7MHz = 2;
inline constexpr uint32_t T0L_7MHz = 6;
inline constexpr uint32_t RESET_7MHz = 51 * 7;

// 2.5MHz 2812 only
inline constexpr uint32_t T1H_2M5Hz = 2; // 800ns
inline constexpr uint32_t T1L_2M5Hz = 1; // 400ns
inline constexpr uint32_t T0H_2M5Hz = 1; // 400ns
inline constexpr uint32_t T0L_2M5Hz = 2; // 800ns
inline constexpr uint32_t RESET_2M5Hz = uint32_t(51.f * 2.5f);

/**
 * @brief base class for LEDs defining color array for given count
 *
 * @tparam COUNT count of LEDs
 */
template <size_t COUNT>
class Leds_base {
public:
    constexpr Leds_base()
        : leds_to_rewrite(COUNT) {
        leds.fill(0);
    }
    using color_array = std::array<uint32_t, COUNT>;

    void Set(uint32_t color, size_t index) {
        if (index >= COUNT) {
            return;
        }

        if (leds[index] == color) {
            return;
        }

        leds[index] = color;
        leds_to_rewrite = std::max(index + size_t(1), leds_to_rewrite);
    }

    void Set(const uint32_t *colors, size_t count) {
        count = std::min(count, size_t(COUNT - 1));
        for (size_t led = 0; led <= count; ++led) {
            Set(colors[led], led);
        }
    }

    void Set(const color_array &colors) {
        Set(colors.begin(), colors.size());
    }

    void ForceRefresh(size_t cnt) {
        leds_to_rewrite = cnt;
    }

protected:
    color_array leds;
    size_t leds_to_rewrite;
    bool force_refresh;
};

/**
 * @brief base class for SPI LEDs handling conversion LEDs to bitset
 *
 * @tparam COUNT count of LEDs
 * @tparam T1H   lenght of high bus status of converted logical "1" bit value
 * @tparam T1L   lenght of low  bus status of converted logical "1" bit value
 * @tparam T0H   lenght of high bus status of converted logical "0" bit value
 * @tparam T0L   lenght of low  bus status of converted logical "0" bit value
 */
template <size_t COUNT, size_t T1H, size_t T1L, size_t T0H, size_t T0L>
class LedsSPI_base : public Leds_base<COUNT> {
public:
    using color_array = std::array<uint32_t, COUNT>;

    constexpr LedsSPI_base() = default;

protected:
    static constexpr uint32_t max_pulse_len = T1H + T1L > T0H + T0L ? T1H + T1L : T0H + T0L; // std::max( T1H + T1L,  T0H + T0L); - not constexpr
    static constexpr uint32_t max_size = max_pulse_len * 24; // 3*8bit color
    static constexpr uint32_t bitfield_size = COUNT * max_size; // theoretical maximum size in bits
    std::bitset<bitfield_size> led_bitset;

    void setHi(size_t &rBitfieldPos) {
        rBitfieldPos += T1L; // no need to set false

        for (size_t i = 0; i < T1H; ++i) {
            led_bitset[rBitfieldPos++] = true;
        }
    }

    void setLo(size_t &rBitfieldPos) {
        rBitfieldPos += T0L; // no need to set false

        for (size_t i = 0; i < T0H; ++i) {
            led_bitset[rBitfieldPos++] = true;
        }
    }

    /**
     * @brief Set the Bitset object from leds array
     *        call exactly once before sending data to LEDs via SPI
     *
     * @return size_t number of bits to be send
     */
    size_t setBitset();
};

template <size_t COUNT, size_t T1H, size_t T1L, size_t T0H, size_t T0L>
size_t LedsSPI_base<COUNT, T1H, T1L, T0H, T0L>::setBitset() {
    if (this->leds_to_rewrite == 0) {
        return 0; // nothing to set
    }

    // Optimization via leds_to_rewrite does not work correctly
    // thanks to LedsSPI_MSB inverting the indexing and breaking the daisy-chaining.
    // As a quick fix, we always update everything
    // BFW-5067 - Someone please fix this :(
    this->leds_to_rewrite = COUNT;

    led_bitset.reset(); // clear bit array

    size_t bitfield_position = 0;

    for (size_t i = 0; i < this->leds_to_rewrite; ++i) {
        std::bitset<24> bits_of_color = this->leds[i];
        for (size_t bit = 0; bit < 24; ++bit) {
            bits_of_color[bit] ? setHi(bitfield_position) : setLo(bitfield_position); // bitfield_position passed by reference
        }
    }

    this->leds_to_rewrite = 0;
    return bitfield_position;
}

/**
 * @brief child of LedsSPI_base handling LSB data conversion
 *
 * @tparam COUNT count of leds
 * @tparam T1H   lenght of high bus status of converted logical "1" bit value
 * @tparam T1L   lenght of low  bus status of converted logical "1" bit value
 * @tparam T0H   lenght of high bus status of converted logical "0" bit value
 * @tparam T0L   lenght of low  bus status of converted logical "0" bit value
 * @tparam RESET_PULSE number of pulses needed to be in low state to write signals on LEDs
 */
// This class is not used at all and should probably be thrown out of the codee -> BFW-5067
template <size_t COUNT, size_t T1H, size_t T1L, size_t T0H, size_t T0L, size_t RESET_PULSE>
class LedsSPI_LSB : public LedsSPI_base<COUNT, T1H, T1L, T0H, T0L> {
protected:
    uint8_t send_buff[(LedsSPI_base<COUNT, T1H, T1L, T0H, T0L>::bitfield_size + RESET_PULSE + 7) / 8];

    /**
     * @brief ready data to send
     *        call exactly once before sending data to LEDs via SPI
     *
     * @return size_t number of bytes to be send
     */
    size_t bitfieldToSendBuff() {
        size_t bit_count = this->setBitset();
        size_t bit_read_index = 0; // from bitset

        // write reset pulse
        for (size_t i = 0; i < (RESET_PULSE + 7) / 8; ++i) {
            send_buff[i] = 0;
        }

        // clear last byte
        send_buff[((bit_count + RESET_PULSE + 7) / 8) - 1] = 0;

        for (; bit_read_index < bit_count; ++bit_read_index) {
            size_t bit_write_index = bit_read_index + RESET_PULSE;
            uint8_t &r_target_byte = send_buff[bit_write_index / 8];
            uint8_t target_bit = 1 << (bit_write_index % 8);
            r_target_byte = this->led_bitset[bit_read_index] ? r_target_byte | target_bit : r_target_byte & (~target_bit);
        }

        return (bit_count + RESET_PULSE + 7) / 8;
    };
};

/**
 * @brief child of LedsSPI_base handling MSB data conversion
 *
 * @tparam COUNT count of leds
 * @tparam T1H   lenght of high bus status of converted logical "1" bit value
 * @tparam T1L   lenght of low  bus status of converted logical "1" bit value
 * @tparam T0H   lenght of high bus status of converted logical "0" bit value
 * @tparam T0L   lenght of low  bus status of converted logical "0" bit value
 * @tparam RESET_PULSE number of pulses needed to be in low state to write signals on LEDs
 */
template <size_t COUNT, size_t T1H, size_t T1L, size_t T0H, size_t T0L, size_t RESET_PULSE>
class LedsSPI_MSB : public LedsSPI_base<COUNT, T1H, T1L, T0H, T0L> {
protected:
    uint8_t send_buff[(LedsSPI_base<COUNT, T1H, T1L, T0H, T0L>::bitfield_size + RESET_PULSE + 7) / 8];

    /**
     * @brief ready data to send
     *        call exactly once before sending data to LEDs via SPI
     *
     * @return size_t number of bytes to be send
     */
    size_t bitfieldToSendBuff() {
        const size_t bit_count = this->setBitset();
        if (!bit_count) {
            return 0;
        }

        // write reset pulse (clear each byt it overlaps)
        for (size_t i = bit_count / 8; i < ((bit_count + RESET_PULSE + 7) / 8); ++i) {
            send_buff[i] = 0;
        }

        size_t bit_read_index = bit_count - 1;
        size_t bit_write_index = 0;

        // This function reverses completely everything,
        // which screws up neopixel driver indexing (doesn't correspond with the daisy-chain order)
        // BFW-5067

        for (; bit_write_index < bit_count; ++bit_write_index, --bit_read_index) {
            uint8_t &r_target_byte = send_buff[bit_write_index / 8];
            uint8_t target_bit = 1 << (7 - (bit_write_index % 8));
            r_target_byte = this->led_bitset[bit_read_index] ? r_target_byte | target_bit : r_target_byte & (~target_bit);
        }

        return (bit_count + RESET_PULSE + 7) / 8;
    };
};

/**
 * @brief draw function pointer
 *
 */
using draw_fn_t = void (*)(uint8_t *, uint16_t);

/**
 * @brief fully functional child of LedsSPI_MSB able to set physical LEDs via pointer
 *        to function hadling SPI DMA
 * @tparam COUNT count of leds
 * @tparam DRAW_FN pointer to draw function
 * @tparam T1H   lenght of high bus status of converted logical "1" bit value
 * @tparam T1L   lenght of low  bus status of converted logical "1" bit value
 * @tparam T0H   lenght of high bus status of converted logical "0" bit value
 * @tparam T0L   lenght of low  bus status of converted logical "0" bit value
 * @tparam RESET_PULSE number of pulses needed to be in low state to write signals on LEDs
 */
template <size_t COUNT, draw_fn_t DRAW_FN, size_t T1H, size_t T1L, size_t T0H, size_t T0L, size_t RESET_PULSE>
class LedsSPI : public LedsSPI_MSB<COUNT, T1H, T1L, T0H, T0L, RESET_PULSE> {
public:
    void Tick() {
        if (!this->leds_to_rewrite) {
            return;
        }

        size_t bytes = this->bitfieldToSendBuff();
        DRAW_FN(this->send_buff, bytes);
    };
};

template <size_t COUNT, draw_fn_t DRAW_FN>
using SPI_21MHz = LedsSPI<COUNT, DRAW_FN, T1H_21MHz, T1L_21MHz, T0H_21MHz, T0L_21MHz, RESET_21MHz>;

template <size_t COUNT, draw_fn_t DRAW_FN>
using SPI_10M5Hz = LedsSPI<COUNT, DRAW_FN, T1H_10M5Hz, T1L_10M5Hz, T0H_10M5Hz, T0L_10M5Hz, RESET_10M5Hz>;

template <size_t COUNT, draw_fn_t DRAW_FN>
using SPI_2M5Hz = LedsSPI<COUNT, DRAW_FN, T1H_2M5Hz, T1L_2M5Hz, T0H_2M5Hz, T0L_2M5Hz, RESET_2M5Hz>;

}; // namespace neopixel
