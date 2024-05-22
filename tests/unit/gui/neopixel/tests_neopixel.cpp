#include "catch2/catch.hpp"
#include "../../../src/hw/neopixel.hpp"
#include <array>
#include <vector>
#include <string>
#include <algorithm>

using namespace neopixel;

constexpr uint8_t bit_reverse8(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

constexpr size_t leds_count = 3;

static std::vector<uint8_t> cmp_vector;

void spi_wr_bytes(uint8_t *pb, uint16_t size) {
    std::vector<uint8_t> v(pb, pb + size / sizeof(pb[0]));
    REQUIRE(cmp_vector == v);
}

template <size_t T1H, size_t T1L, size_t T0H, size_t T0L, size_t RESET_TIME, size_t LED_COUNT> // LED_COUNT must be last to be deducted
void led_test(typename Leds_base<LED_COUNT>::color_array colors, const std::vector<uint8_t> &result) {
    cmp_vector = result;

    neopixel::LedsSPI<LED_COUNT, spi_wr_bytes, T1H, T1L, T0H, T0L, RESET_TIME> led;

    led.Set(colors);
    led.Tick();
}

static std::string cmp_string;

// TODO asymmetry T1H + T1L > T0H + T0L ? T1H + T1L : T0H + T0L
template <size_t SZ>
void spi_wr_bytes__string(uint8_t *pb, uint16_t size) {
    std::bitset<SZ> bs;
    for (size_t i = 0; i < size; ++i) {
        bs <<= 8;
        std::bitset<SZ> x((unsigned long)(pb[i]));
        bs |= x;
    }

    size_t add_front_zeroes = size_t(SZ - size * 8); // difference between maximum current size
    cmp_string.insert(0, add_front_zeroes, '0');
    cmp_string.resize(SZ, '0'); // resize to be comparable with bitfield, fill unused fields with zeroes
    REQUIRE(cmp_string == bs.to_string());
}

template <size_t T1H, size_t T1L, size_t T0H, size_t T0L, size_t RESET_TIME, size_t LED_COUNT> // LED_COUNT must be last to be deducted
void led_test(typename Leds_base<LED_COUNT>::color_array colors, const std::string &result) {
    cmp_string = result;
    static constexpr size_t SZ = (((T1H + T1L > T0H + T0L ? T1H + T1L : T0H + T0L) * LED_COUNT * 8 * 3 + RESET_TIME + 7) / 8) * 8; // 3 color 8 bit
    neopixel::LedsSPI<LED_COUNT, spi_wr_bytes__string<SZ>, T1H, T1L, T0H, T0L, RESET_TIME> led;

    led.Set(colors);
    led.Tick();
}

// replace '1' with T1H * '1' and with T1L * '0'
// replace '0' with T0H * '1' and with T0L * '0'
std::string string_extend(std::string s, size_t T1H, size_t T1L, size_t T0H, size_t T0L) {
    // remove all spaces
    s.erase(remove_if(s.begin(), s.end(), isspace), s.end());

    // replace all '1' with 'T' and '0' with 'F'
    size_t pos = std::string::npos;
    while ((pos = s.find_last_of('1', pos)) != std::string::npos) {
        s.replace(pos, 1, 1, 'T');
    }
    pos = std::string::npos;
    while ((pos = s.find_last_of('0', pos)) != std::string::npos) {
        s.replace(pos, 1, 1, 'F');
    }

    std::string true_sequence = "";
    for (size_t i = 0; i < T1H; ++i) {
        true_sequence.push_back('1');
    }
    for (size_t i = 0; i < T1L; ++i) {
        true_sequence.push_back('0');
    }

    std::string false_sequence = "";
    for (size_t i = 0; i < T0H; ++i) {
        false_sequence.push_back('1');
    }
    for (size_t i = 0; i < T0L; ++i) {
        false_sequence.push_back('0');
    }

    // replace all 'T' with "true_sequence" (for example "1111100") and 'F' with "false_sequence" (for example "1000")
    pos = std::string::npos;
    while ((pos = s.find_last_of('T', pos)) != std::string::npos) {
        s.replace(pos, 1, true_sequence);
    }
    pos = std::string::npos;
    while ((pos = s.find_last_of('F', pos)) != std::string::npos) {
        s.replace(pos, 1, false_sequence);
    }
    return s;
}

TEST_CASE("LEDS") {
    cmp_vector.clear();
    SECTION("1 led") {
        Leds_base<1>::color_array cl0 = { { 0 } };
        led_test<1, 0, 0, 1, 0, 1>(cl0, std::vector<uint8_t>({ 0, 0, 0 }));
        Leds_base<1>::color_array cl1 = { { 0x01 } };
        led_test<1, 0, 0, 1, 0, 1>(cl1, std::vector<uint8_t>({ 0, 0, 0x01 }));
        Leds_base<1>::color_array colors = { { 0x00123456 } };
        led_test<1, 0, 0, 1, 0, 1>(colors, std::vector<uint8_t>({ 0x12, 0x34, 0x56 }));
        led_test<1, 0, 0, 1, 1, 1>(colors, std::vector<uint8_t>({ 0x12, 0x34, 0x56, 0 })); // add one bit (rounded up to bytes) and fill with zeroes
        led_test<1, 0, 0, 1, 16, 1>(colors, std::vector<uint8_t>({ 0x12, 0x34, 0x56, 0, 0 })); // add 16 bit (rounded up to bytes) and fill with zeroes
        led_test<1, 1, 1, 1, 0, 1>(cl0, std::vector<uint8_t>({ 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010 })); // set both logical 1 and 0 to generate signal "10"

        // 0001 0010  0011 0100  0101 0110
        // 8bit must be MSB
        std::vector<uint8_t> result(
            {
                0,
                0,
                0,
                0xff,
                0,
                0,
                0xff,
                0,

                0,
                0,
                0xff,
                0xff,
                0,
                0xff,
                0,
                0,

                0,
                0xff,
                0,
                0xff,
                0,
                0xff,
                0xff,
                0,
            });
        led_test<8, 0, 0, 8, 0, 1>(colors, result);

        // 16bit 14-2 : 2-14
#define ZER 0xc0, 0x00
#define ONE 0xff, 0xfc
        result = std::vector<uint8_t>(
            {
                ZER,
                ZER,
                ZER,
                ONE,
                ZER,
                ZER,
                ONE,
                ZER,

                ZER,
                ZER,
                ONE,
                ONE,
                ZER,
                ONE,
                ZER,
                ZER,

                ZER,
                ONE,
                ZER,
                ONE,
                ZER,
                ONE,
                ONE,
                ZER,
            });
        led_test<14, 2, 2, 14, 0, 1>(colors, result);
    }

    // 10.5 MHz
    static constexpr size_t T1H = 7;
    static constexpr size_t T1L = 6;
    static constexpr size_t T0H = 3;
    static constexpr size_t T0L = 9;
    static constexpr size_t END_PULSE = uint32_t(51.f * 10.5f);

    SECTION("real format black") {
        Leds_base<1>::color_array colors = { { 0x00000000 } };

        static constexpr const char *input_data = "00000000 00000000 00000000"; // color .. TODO autoconvert
        std::string result_string = string_extend(input_data, T1H, T1L, T0H, T0L);
        led_test<T1H, T1L, T0H, T0L, END_PULSE, 1>(colors, result_string);
    }

    SECTION("real format red") {
        Leds_base<1>::color_array colors = { { 0x000000ff } };

        static constexpr const char *input_data = "00000000 00000000 11111111";
        std::string result_string = string_extend(input_data, T1H, T1L, T0H, T0L);
        led_test<T1H, T1L, T0H, T0L, END_PULSE, 1>(colors, result_string);
    }

    SECTION("real format green") {
        Leds_base<1>::color_array colors = { { 0x0000ff00 } };

        static constexpr const char *input_data = "00000000 11111111 00000000";
        std::string result_string = string_extend(input_data, T1H, T1L, T0H, T0L);
        led_test<T1H, T1L, T0H, T0L, END_PULSE, 1>(colors, result_string);
    }

    SECTION("real format blue") {
        Leds_base<1>::color_array colors = { { 0x00ff0000 } };

        static constexpr const char *input_data = "11111111 00000000 00000000";
        std::string result_string = string_extend(input_data, T1H, T1L, T0H, T0L);
        led_test<T1H, T1L, T0H, T0L, END_PULSE, 1>(colors, result_string);
    }

    SECTION("real format 4 leds") {
        Leds_base<4>::color_array colors = { { 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 } };

        // after neopixel receives 24 bits, they are stored, than rest is shifted out
        // this behavior mirrors order of LEDs
        static constexpr const char *input_data = "00000000 00000000 00000000  11111111 00000000 00000000  00000000 11111111 00000000  00000000 00000000 11111111";
        std::string result_string = string_extend(input_data, T1H, T1L, T0H, T0L);
        led_test<T1H, T1L, T0H, T0L, END_PULSE, 4>(colors, result_string);
    }
}
