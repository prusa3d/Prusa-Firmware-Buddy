// convert int32_t to c string
#pragma once

#include "stdint.h"
#include "stdlib.h"

/*****************************************************************************/
// intended api
extern constexpr bool is_negative(int32_t i);
extern constexpr size_t count_digits(int32_t i);
extern constexpr char nth_char(int32_t i, size_t n);
extern constexpr int pow_int(int val, size_t pow);
extern constexpr int remove_front_digit(int32_t i);
/*****************************************************************************/

// 0 == 0 digits
static constexpr size_t _count_digits_unsigned(uint32_t ui) {
    return ui > 0 ? _count_digits_unsigned(ui / 10) + 1 : 0;
}

// 0 == 1 digits
static constexpr size_t count_digits_unsigned(uint32_t ui) {
    return ui == 0 ? 1 : _count_digits_unsigned(ui);
}

constexpr bool is_negative(int32_t i) {
    return i < 0;
}

constexpr size_t count_digits(int32_t i) {
    return is_negative(i) ? count_digits_unsigned(-i) + 1 : count_digits_unsigned(i);
}

// n must be valid
static constexpr char valid_nth_char_unsigned(uint32_t ui, size_t n) {
    return n == count_digits_unsigned(ui) - 1 ? (ui % 10) + '0' : valid_nth_char_unsigned(ui / 10, n);
}

// return '/0' if n is too big
static constexpr char nth_char_unsigned(uint32_t ui, size_t n) {
    return n >= count_digits_unsigned(ui) ? 0 : valid_nth_char_unsigned(ui, n);
}

// returns nth char of -ui
static constexpr char nth_char_negative(int32_t i, size_t n) {
    return n == 0 ? '-' : nth_char_unsigned(-i, n - 1);
}

constexpr char nth_char(int32_t i, size_t n) {
    return i < 0 ? nth_char_negative(i, n) : nth_char_unsigned(i, n);
}

constexpr int pow_int(int val, size_t pow) {
    return pow == 0 ? 1 : val * pow_int(val, pow - 1);
}

static constexpr int remove_front_digit_unsigned(uint32_t ui) {
    return ui % pow_int(10, count_digits_unsigned(ui));
}

constexpr int remove_front_digit(int32_t i) {
    return i < 0 ? -1 : remove_front_digit_unsigned(i);
}
